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

}  // namespace compass_nav2
