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

#include "compass_nav2/costmap_env_query.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "nav2_costmap_2d/cost_values.hpp"

namespace compass_nav2
{

void CostmapEnvQuery::setContext(
  const nav2_costmap_2d::Costmap2D * costmap,
  const compass::SE2 & robot_pose,
  const compass::Point2D & local_goal,
  const std::vector<compass::Person> & people,
  const compass::Twist2D & robot_vel)
{
  costmap_ = costmap;
  robot_pose_ = robot_pose;
  local_goal_ = local_goal;
  people_ = people;
  robot_vel_ = robot_vel;
}

double CostmapEnvQuery::lateral_bias(const compass::TopoClass & c) const
{
  if (c.size() == 0) {
    return 0.0;
  }
  double sum = 0.0;
  for (const auto & [id, side] : c.pairs()) {
    (void)id;
    sum += (side == compass::Side::R) ? 1.0 : -1.0;
  }
  return sum / static_cast<double>(c.size());
}

bool CostmapEnvQuery::occupied(double wx, double wy) const
{
  if (costmap_ == nullptr) {
    return false;
  }
  unsigned int mx = 0;
  unsigned int my = 0;
  if (!costmap_->worldToMap(wx, wy, mx, my)) {
    // costmap 밖은 미지 영역 — 보존적으로 비점유로 본다(통과 가능 측).
    return false;
  }
  const unsigned char cost = costmap_->getCost(mx, my);
  return cost >= nav2_costmap_2d::INSCRIBED_INFLATED_OBSTACLE &&
         cost != nav2_costmap_2d::NO_INFORMATION;
}

double CostmapEnvQuery::clearance(const compass::TopoClass & c) const
{
  if (costmap_ == nullptr) {
    return kDefaultClearance;
  }
  // 로봇에서 로컬 목표 방향으로, class 의 횡 오프셋만큼 옆으로 비킨 ray 를 쏜다.
  const double dgx = local_goal_.x - robot_pose_.x;
  const double dgy = local_goal_.y - robot_pose_.y;
  double heading = std::atan2(dgy, dgx);
  if (dgx == 0.0 && dgy == 0.0) {
    heading = robot_pose_.theta;
  }
  const double bias = lateral_bias(c) * kLateralStep;
  const double ox = robot_pose_.x + bias * std::cos(heading + M_PI_2);
  const double oy = robot_pose_.y + bias * std::sin(heading + M_PI_2);

  const double res = std::max(costmap_->getResolution(), 1e-3);
  for (double s = 0.0; s <= kRayMax; s += res) {
    const double px = ox + s * std::cos(heading);
    const double py = oy + s * std::sin(heading);
    if (occupied(px, py)) {
      return s;
    }
  }
  return kRayMax;
}

double CostmapEnvQuery::corridor_width(const compass::TopoClass & c) const
{
  if (costmap_ == nullptr) {
    return kDefaultCorridor;
  }
  const double dgx = local_goal_.x - robot_pose_.x;
  const double dgy = local_goal_.y - robot_pose_.y;
  double heading = std::atan2(dgy, dgx);
  if (dgx == 0.0 && dgy == 0.0) {
    heading = robot_pose_.theta;
  }
  const double bias = lateral_bias(c) * kLateralStep;
  // 진행선 위 한 점(robot 전방 1 m 부근)에서 좌·우로 lethal cell 까지 폭 측정.
  const double cx = robot_pose_.x + bias * std::cos(heading + M_PI_2) + std::cos(heading);
  const double cy = robot_pose_.y + bias * std::sin(heading + M_PI_2) + std::sin(heading);
  const double res = std::max(costmap_->getResolution(), 1e-3);
  const double half_max = kDefaultCorridor;  // 한쪽 탐색 상한

  auto side_dist = [&](double sign) {
    for (double d = 0.0; d <= half_max; d += res) {
      const double px = cx + sign * d * std::cos(heading + M_PI_2);
      const double py = cy + sign * d * std::sin(heading + M_PI_2);
      if (occupied(px, py)) {
        return d;
      }
    }
    return half_max;
  };
  return side_dist(+1.0) + side_dist(-1.0);
}

double CostmapEnvQuery::ttc(const compass::TopoClass & c) const
{
  (void)c;
  // 사람 상대 접근에서 최소 TTC 산출. 사람이 없으면 보존적으로 큰 값.
  double min_ttc = kDefaultTtc;
  for (const auto & p : people_) {
    const double dx = p.pose.x - robot_pose_.x;
    const double dy = p.pose.y - robot_pose_.y;
    const double dist = std::hypot(dx, dy);
    if (dist < 1e-3) {
      return 0.0;
    }
    // 상대 속도 (로봇 - 사람)을 시선 방향에 사영한 접근 속도.
    // 사람 속도는 Twist2D(vx=전진 속력) — 사람 헤딩으로 평면 분해한다.
    const double pvx = p.vel.vx * std::cos(p.pose.theta);
    const double pvy = p.vel.vx * std::sin(p.pose.theta);
    const double rvx = robot_vel_.vx * std::cos(robot_pose_.theta) - pvx;
    const double rvy = robot_vel_.vx * std::sin(robot_pose_.theta) - pvy;
    const double closing = -(rvx * dx + rvy * dy) / dist;  // +면 접근
    if (closing > 1e-3) {
      min_ttc = std::min(min_ttc, dist / closing);
    }
  }
  return min_ttc;
}

bool CostmapEnvQuery::feasible(const compass::TopoClass & c) const
{
  // 하드 실현 가능성: 해당 class 횡 오프셋의 즉시 전방이 lethal 이 아니어야 한다.
  if (costmap_ == nullptr) {
    return true;
  }
  return clearance(c) > std::max(costmap_->getResolution(), 1e-3);
}

}  // namespace compass_nav2
