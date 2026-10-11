// Copyright (c) 2026 Kang Jung Mo
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "compass_nav2/path_tracking.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace compass_nav2
{

namespace
{
double wrapPi(double a)
{
  while (a > M_PI) {a -= 2.0 * M_PI;}
  while (a < -M_PI) {a += 2.0 * M_PI;}
  return a;
}
}  // namespace

double pathTrackingAngularZ(
  const compass::SE2 & robot,
  const std::vector<compass::Point2D> & path,
  double side_bias,
  const PathTrackGains & gains)
{
  const double side = gains.k_side * side_bias;

  // 점이 없으면 횡오차 기준이 없어 측면 편향만 적용한다 (사실상 직진).
  if (path.empty()) {
    return std::clamp(side, -gains.max_w, gains.max_w);
  }

  // 점이 하나면 그 점을 향한 헤딩 보정만 적용한다.
  if (path.size() == 1) {
    const double psi = wrapPi(
      robot.theta - std::atan2(path[0].y - robot.y, path[0].x - robot.x));
    return std::clamp(-gains.k_theta * psi + side, -gains.max_w, gains.max_w);
  }

  // 가장 가까운 선분을 찾아 부호 있는 횡오차 e 와 접선 헤딩을 구한다.
  double best_d2 = std::numeric_limits<double>::max();
  double e_signed = 0.0;     // 경로 진행 방향 기준 좌측 +, 우측 −
  double tangent = 0.0;      // 가장 가까운 선분의 헤딩 (rad)
  for (std::size_t i = 0; i + 1 < path.size(); ++i) {
    const double ax = path[i].x, ay = path[i].y;
    const double bx = path[i + 1].x, by = path[i + 1].y;
    const double abx = bx - ax, aby = by - ay;
    const double seg_len2 = abx * abx + aby * aby;
    if (seg_len2 < 1e-12) {
      continue;  // 퇴화 선분 건너뜀.
    }
    // 로봇을 선분에 정사영 (파라미터 t 를 [0,1] 로 클램프).
    double t = ((robot.x - ax) * abx + (robot.y - ay) * aby) / seg_len2;
    t = std::clamp(t, 0.0, 1.0);
    const double cx = ax + t * abx, cy = ay + t * aby;
    const double d2 = (robot.x - cx) * (robot.x - cx) + (robot.y - cy) * (robot.y - cy);
    if (d2 < best_d2) {
      best_d2 = d2;
      const double seg_len = std::sqrt(seg_len2);
      // 부호 있는 횡오차 = 단위 접선 × (로봇 − 최근접점) 의 z 성분.
      e_signed = (abx * (robot.y - cy) - aby * (robot.x - cx)) / seg_len;
      tangent = std::atan2(aby, abx);
    }
  }

  const double psi = wrapPi(robot.theta - tangent);
  const double w = -gains.k_e * e_signed - gains.k_theta * psi + side;
  return std::clamp(w, -gains.max_w, gains.max_w);
}

compass::Point2D localGoalAhead(
  const compass::SE2 & robot,
  const std::vector<compass::Point2D> & path,
  double lookahead,
  std::size_t & segment,
  double window)
{
  const std::size_t n = path.size();
  const auto finite = [&path](std::size_t i) {
      return std::isfinite(path[i].x) && std::isfinite(path[i].y);
    };
  // The next finite point after i; n if there is none.
  const auto next = [&finite, n](std::size_t i) {
      for (++i; i < n && !finite(i); ++i) {
      }
      return i;
    };
  std::size_t first_finite = 0;
  while (first_finite < n && !finite(first_finite)) {
    ++first_finite;
  }
  if (first_finite == n) {
    // Empty plan, or no finite point in it.
    segment = 0;
    return {robot.x + lookahead * std::cos(robot.theta),
      robot.y + lookahead * std::sin(robot.theta)};
  }
  if (segment >= n || !finite(segment) || next(segment) >= n) {
    segment = first_finite;  // not the start of a segment of this path
  }

  // Squared distance from the robot to the segment path[i] -> path[j] and the
  // projection parameter t in [0, 1]; false for a segment shorter than 1e-6 m.
  const auto project = [&path, &robot](std::size_t i, std::size_t j, double & d2, double & t) {
      const double abx = path[j].x - path[i].x;
      const double aby = path[j].y - path[i].y;
      const double seg_len2 = abx * abx + aby * aby;
      if (!(seg_len2 >= 1e-12)) {
        return false;
      }
      t = std::clamp(
        ((robot.x - path[i].x) * abx + (robot.y - path[i].y) * aby) / seg_len2, 0.0, 1.0);
      const double cx = path[i].x + t * abx;
      const double cy = path[i].y + t * aby;
      d2 = (robot.x - cx) * (robot.x - cx) + (robot.y - cy) * (robot.y - cy);
      return true;
    };

  // 1) Closest segment, searching forwards: one windowed pass from `first`,
  // repeated from the segment it found until a pass finds nothing closer.
  double best_d2 = std::numeric_limits<double>::infinity();
  double best_t = 0.0;
  for (std::size_t first = segment;; first = segment) {
    double along = 0.0;  // path length from the end of segment `first` to path[i]
    int k = 0;           // 0 for segment `first`, 1 for the one after it, ...
    for (std::size_t i = first, j = next(i); j < n; i = j, j = next(j), ++k) {
      // The segment after `first` is always looked at. Written so that a
      // non-finite length or window also ends the pass.
      if (k > 1 && !(along <= window)) {
        break;
      }
      double d2 = 0.0, t = 0.0;
      if (project(i, j, d2, t) && d2 < best_d2) {
        best_d2 = d2;
        best_t = t;
        segment = i;
      }
      if (k > 0) {
        along += std::hypot(path[j].x - path[i].x, path[j].y - path[i].y);
      }
    }
    if (segment == first) {
      break;
    }
  }

  // 1b) Re-acquisition. The forward search cannot follow a robot that moved
  // back along the plan, was placed on a later pass, or restarted far from the
  // plan start. When the remembered segment is further than `window` from the
  // robot, look at every segment (as the steering term does: the earliest wins
  // a tie) and move there if it is nearer by MORE than `window`. Requiring that
  // margin, not just "nearer", keeps the choice from flapping between two
  // passes of the plan that are about equally far from the robot.
  const double reach = std::max(window, 0.0);
  const double best_d = std::sqrt(best_d2);
  if (!(best_d <= reach)) {
    double all_d2 = std::numeric_limits<double>::infinity();
    double all_t = 0.0;
    std::size_t all_i = segment;
    for (std::size_t i = first_finite, j = next(i); j < n; i = j, j = next(j)) {
      double d2 = 0.0, t = 0.0;
      if (project(i, j, d2, t) && d2 < all_d2) {
        all_d2 = d2;
        all_t = t;
        all_i = i;
      }
    }
    if (best_d - std::sqrt(all_d2) > reach) {
      segment = all_i;
      best_t = all_t;
    }
  }

  // 2) First finite point ahead that is at least `lookahead` from the robot;
  // the last finite point of the plan if there is none. best_t stays 0 when no
  // segment was usable, so the scan then starts at `segment`.
  std::size_t last = segment;
  for (std::size_t j = best_t > 0.0 ? next(segment) : segment; j < n; j = next(j)) {
    if (std::hypot(path[j].x - robot.x, path[j].y - robot.y) >= lookahead) {
      return path[j];
    }
    last = j;
  }
  return path[last];  // the scan ran to the end: the last finite point
}

}  // namespace compass_nav2
