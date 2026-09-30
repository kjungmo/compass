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
  const compass::Twist2D & robot_vel,
  bool use_candidate_trajectories,
  const std::vector<compass::Point2D> & global_path,
  double candidate_speed,
  const PathTrackGains & gains)
{
  costmap_ = costmap;
  robot_pose_ = robot_pose;
  local_goal_ = local_goal;
  people_ = people;
  robot_vel_ = robot_vel;
  use_candidate_trajectories_ = use_candidate_trajectories;
  global_path_ = global_path;
  candidate_speed_ = candidate_speed;
  gains_ = gains;
}

std::optional<compass::CandidateTrajectory> CostmapEnvQuery::candidate_trajectory(
  const compass::TopoClass & c) const
{
  if (!use_candidate_trajectories_) {
    return std::nullopt;
  }
  return trajectoryAtSpeed(c, candidate_speed_);
}

compass::CandidateTrajectory CostmapEnvQuery::trajectoryAtSpeed(
  const compass::TopoClass & c, double speed) const
{
  if (!use_candidate_trajectories_) {
    return {};
  }
  return candidateRollout(robot_pose_, global_path_, c, speed, gains_);
}

double CostmapEnvQuery::lateral_bias(const compass::TopoClass & c) const
{
  if (c.size() == 0) {
    return 0.0;
  }
  double sum = 0.0;
  for (const auto & [id, side] : c.pairs()) {
    (void)id;
    // Match the emitted path-tracking convention: L is positive path-normal.
    sum += (side == compass::Side::R) ? -1.0 : 1.0;
  }
  return sum / static_cast<double>(c.size());
}

bool CostmapEnvQuery::occupied(double wx, double wy) const
{
  if (costmap_ == nullptr) {
    return true;
  }
  unsigned int mx = 0;
  unsigned int my = 0;
  if (!costmap_->worldToMap(wx, wy, mx, my)) {
    // Unobserved space is unavailable, including cells outside this map.
    return true;
  }
  const unsigned char cost = costmap_->getCost(mx, my);
  return cost >= nav2_costmap_2d::INSCRIBED_INFLATED_OBSTACLE;
}

double CostmapEnvQuery::clearance(const compass::TopoClass & c) const
{
  if (use_candidate_trajectories_) {
    const auto trajectory = trajectoryAtSpeed(c, candidate_speed_);
    return trajectory.empty() ? 0.0 : trajectoryClearance(trajectory);
  }
  if (costmap_ == nullptr || occupied(robot_pose_.x, robot_pose_.y)) {
    return 0.0;
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
  if (use_candidate_trajectories_) {
    const double clear = clearance(c);
    return std::isfinite(clear) && clear > 0.0 ?
      2.0 * (clear + kRobotRadius) : 0.0;
  }
  if (costmap_ == nullptr) {
    return 0.0;
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
  if (use_candidate_trajectories_) {
    const auto trajectory = trajectoryAtSpeed(c, candidate_speed_);
    return trajectory.empty() ? 0.0 : trajectoryTtc(trajectory);
  }
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
    const double closing = (rvx * dx + rvy * dy) / dist;  // robot-minus-person: + means approaching
    if (closing > 1e-3) {
      min_ttc = std::min(min_ttc, dist / closing);
    }
  }
  return min_ttc;
}

bool CostmapEnvQuery::feasible(const compass::TopoClass & c) const
{
  if (use_candidate_trajectories_) {
    const auto trajectory = trajectoryAtSpeed(c, candidate_speed_);
    return !trajectory.empty() && trajectoryClearance(trajectory) > 0.0;
  }
  // 하드 실현 가능성: 해당 class 횡 오프셋의 즉시 전방이 lethal 이 아니어야 한다.
  if (costmap_ == nullptr) {
    return false;
  }
  return clearance(c) > std::max(costmap_->getResolution(), 1e-3);
}

double CostmapEnvQuery::trajectoryClearance(
  const compass::CandidateTrajectory & trajectory) const
{
  // Both environment paths fail closed without a map; archived offline metrics
  // do not justify a permissive runtime safety fallback.
  if (costmap_ == nullptr || trajectory.empty()) {
    return 0.0;
  }
  const double res = costmap_->getResolution();
  if (!std::isfinite(res) || res <= 0.0) {
    return 0.0;
  }
  // Samples alone can miss an obstacle between controller ticks. Any point on
  // an exact constant-speed unicycle interval is at most half its arc length
  // from one endpoint. Distance to obstacles/map edges is 1-Lipschitz, so this
  // subtraction makes the sampled minimum a conservative swept lower bound.
  double sweep_padding = 0.0;
  for (size_t i = 1; i < trajectory.size(); ++i) {
    const double dt = trajectory[i].t - trajectory[i - 1].t;
    const double half_arc = 0.5 * std::abs(trajectory[i - 1].command.vx) * dt;
    if (!std::isfinite(dt) || dt <= 0.0 || !std::isfinite(half_arc)) {
      return 0.0;
    }
    sweep_padding = std::max(sweep_padding, half_arc);
  }
  // Clearance is already capped at kClearanceScan; huge intervals cannot be
  // certified by this bounded scan and must fail closed.
  if (sweep_padding >= kClearanceScan) {
    return 0.0;
  }
  const double min_x = costmap_->getOriginX();
  const double min_y = costmap_->getOriginY();
  const double max_x = min_x + costmap_->getSizeInMetersX();
  const double max_y = min_y + costmap_->getSizeInMetersY();
  double minimum = kClearanceScan;
  const int cells = static_cast<int>(std::ceil((kClearanceScan + kRobotRadius) / res));
  for (const auto & sample : trajectory) {
    const double x = sample.pose.x;
    const double y = sample.pose.y;
    if (!std::isfinite(x) || !std::isfinite(y)) {
      return 0.0;
    }
    // Distance to map boundary is part of clearance: an out-of-map footprint
    // cannot pass merely because worldToMap omits it.
    const double edge = std::min({x - min_x, max_x - x, y - min_y, max_y - y}) - kRobotRadius;
    if (edge <= 0.0) {
      return 0.0;
    }
    minimum = std::min(minimum, edge);
    unsigned int mx = 0, my = 0;
    if (!costmap_->worldToMap(x, y, mx, my)) {
      return 0.0;
    }
    const int ix0 = std::max(0, static_cast<int>(mx) - cells);
    const int iy0 = std::max(0, static_cast<int>(my) - cells);
    const int ix1 = std::min(static_cast<int>(costmap_->getSizeInCellsX()) - 1,
      static_cast<int>(mx) + cells);
    const int iy1 = std::min(static_cast<int>(costmap_->getSizeInCellsY()) - 1,
      static_cast<int>(my) + cells);
    for (int ix = ix0; ix <= ix1; ++ix) {
      for (int iy = iy0; iy <= iy1; ++iy) {
        const auto cost = costmap_->getCost(ix, iy);
        // Own the circular footprint here, so count source-obstacle cells rather
        // than subtracting the radius again from an inflation-layer footprint.
        if (cost < nav2_costmap_2d::LETHAL_OBSTACLE &&
            cost != nav2_costmap_2d::NO_INFORMATION) {
          continue;
        }
        double wx = 0.0, wy = 0.0;
        costmap_->mapToWorld(ix, iy, wx, wy);
        // Subtract half the cell diagonal so a cell is treated as area, not a point.
        const double d = std::hypot(wx - x, wy - y) - kRobotRadius - res / std::sqrt(2.0);
        minimum = std::min(minimum, std::max(0.0, d));
      }
    }
  }
  return std::max(0.0, minimum - sweep_padding);
}

double CostmapEnvQuery::trajectoryTtc(
  const compass::CandidateTrajectory & trajectory) const
{
  if (trajectory.empty()) {
    return 0.0;
  }
  double minimum = kDefaultTtc;
  for (const auto & sample : trajectory) {
    for (const auto & person : people_) {
      if (!std::isfinite(person.pose.x) || !std::isfinite(person.pose.y) ||
          !std::isfinite(person.pose.theta) || !std::isfinite(person.vel.vx)) {
        return 0.0;
      }
      const double pvx = person.vel.vx * std::cos(person.pose.theta);
      const double pvy = person.vel.vx * std::sin(person.pose.theta);
      const double px = person.pose.x + pvx * sample.t;
      const double py = person.pose.y + pvy * sample.t;
      const double dx = px - sample.pose.x;
      const double dy = py - sample.pose.y;
      const double center_dist = std::hypot(dx, dy);
      const double separation = center_dist - kRobotRadius - kPersonRadius;
      if (separation <= 0.0 || center_dist < 1e-9) {
        return std::min(minimum, sample.t);
      }
      const double rvx = sample.command.vx * std::cos(sample.pose.theta);
      const double rvy = sample.command.vx * std::sin(sample.pose.theta);
      const double relative_x = pvx - rvx;
      const double relative_y = pvy - rvy;
      const double closing = -(dx * relative_x + dy * relative_y) / center_dist;
      if (closing > 1e-6) {
        minimum = std::min(minimum, sample.t + separation / closing);
      }
    }
  }
  return minimum;
}

bool CostmapEnvQuery::executionSafe(
  const compass::TopoClass & c, double speed, double d_safe, double ttc_min) const
{
  if (!std::isfinite(d_safe) || d_safe < 0.0 ||
      !std::isfinite(ttc_min) || ttc_min < 0.0) {
    return false;
  }
  const auto trajectory = trajectoryAtSpeed(c, speed);
  if (trajectory.empty()) {
    return false;
  }
  const double clear = trajectoryClearance(trajectory);
  // Zero clearance is infeasible even if a caller explicitly configures
  // d_safe=0; touching an obstacle or leaving the map is never executable.
  return clear > 0.0 && clear >= d_safe && trajectoryTtc(trajectory) >= ttc_min;
}

}  // namespace compass_nav2
