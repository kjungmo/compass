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

#ifndef COMPASS_NAV2__PATH_TRACKING_HPP_
#define COMPASS_NAV2__PATH_TRACKING_HPP_

#include <cstddef>
#include <vector>

#include "compass_core/types.hpp"

namespace compass_nav2
{

// 경로 추종 조향 이득 (cross-track + heading PD).
//
// 선형화 폐루프는 ë + k_theta·ė + (v·k_e)·e = 0 이라, 고유진동수 ωn=√(v·k_e),
// 감쇠비 ζ=k_theta/(2√(v·k_e)) 이다. 임계 감쇠는 k_theta = 2√(v·k_e). 기본값
// (k_e=3, k_theta=3, v≈0.45)은 ζ≈1.3 의 과감쇠로, 직선 복도에서 오버슈트·위빙
// 없이 수렴한다. (예전의 베어링 비례 조향은 ζ<1 로 ±0.8 m 위빙을 일으켰다.)
struct PathTrackGains
{
  double k_e = 3.0;        // cross-track(횡오차) 이득 (1/m·rad/s)
  double k_theta = 3.0;    // heading(헤딩오차) 이득 (rad/s per rad)
  double k_side = 0.4;     // 사회적 측면 편향 이득 (rad/s per unit bias)
  double max_w = 1.0;      // yaw rate 상한 (rad/s)
};

// 경로 폴리라인을 따라가는 조향 각속도(angular.z, rad/s)를 산출한다.
//
// 동작: 로봇에서 가장 가까운 경로 선분을 찾아 (a) 부호 있는 횡오차 e (경로
// 진행 방향 기준 좌측 +, 우측 −) 와 (b) 그 선분의 접선 헤딩을 구한 뒤, 헤딩
// 오차 ψ = wrap(robot.theta − 접선) 과 함께 PD 로 조향한다:
//     ω = clamp(−k_e·e − k_theta·ψ + k_side·side_bias, ±max_w)
// 횡오차·헤딩오차를 직접 되먹임하므로 직선 경로에 임계/과감쇠로 수렴한다.
// side_bias 항은 결정 코어가 고른 통과 측면(L +, R −)으로의 약한 횡 편향을
// 더해, 보행자 교차 시 angular.z 부호 전환(사회적 커밋)을 보존한다.
//
// path 는 global_frame 폴리라인, robot 은 같은 프레임의 자세, side_bias ∈ [−1,1].
// 점이 0개면 side_bias 항만, 1개면 그 점을 향한 헤딩 보정만 적용한다.
// node 가 필요 없는 자유 함수이므로 단위 테스트가 가능하다.
double pathTrackingAngularZ(
  const compass::SE2 & robot,
  const std::vector<compass::Point2D> & path,
  double side_bias,
  const PathTrackGains & gains);

// How far past a segment the closest-segment search looks for a closer one, as
// path length (m); also how far the remembered segment may be from the robot,
// and by how much another segment must be nearer, before the whole plan is
// searched again. See localGoalAhead.
inline constexpr double kPlanSearchWindow = 1.0;

// Local goal for the decision core: the first plan point AHEAD of the robot along
// the plan that is at least `lookahead` (m, straight-line) from the robot.
//
// `segment` is the caller's memory of where the robot is along this plan: the
// index i of the point where the segment found closest by the last call starts
// (it runs from path[i] to the next finite point). Set it to 0 whenever the
// plan geometry changes or a new task starts.
//
//   1. Closest segment, searched forwards. Starting at `segment`: look at it,
//      the one after it, and later segments that start within `window` of path
//      length after its end; move to the closest of those (the earliest wins a
//      tie) and repeat from there until nothing in range is closer. A robot
//      that follows the plan, or has jumped ahead, is found this way, and a
//      later pass of a plan that crosses itself or runs back along itself is
//      not taken while it is more than `window` of path length away.
//  1b. Re-acquisition. If the segment found by 1 is further than `window` from
//      the robot, every segment of the plan is looked at (the earliest wins a
//      tie, as in pathTrackingAngularZ) and the nearest one replaces it when it
//      is nearer by MORE than `window`. This is what finds a robot that moved
//      back along the plan by more than `window`, was restarted away from the
//      plan start, or follows a later pass of a self-crossing plan. The margin
//      is the hysteresis: two passes about equally far from the robot do not
//      take turns. It never runs while the robot is within `window` of the
//      remembered segment, so normal tracking is the forward search alone.
//   2. From that segment's far end (its near end when the robot projects on or
//      before the near end), the first point at straight-line distance >=
//      lookahead from the robot; the last finite plan point if there is none.
//
// `segment` is set to the result of 1 and 1b. It decreases only in 1b.
//
// While every plan point behind the robot is nearer than `lookahead` (a plan
// issued from the robot pose and replaced before the robot has gone `lookahead`
// along it), this is the first plan point at distance >= lookahead counted from
// the plan start: the rule used before `segment` existed.
//
// Non-finite points (NaN or infinite x or y) are treated as absent: segments
// join the finite points on either side, and such a point is never returned.
// Empty path, or no finite point: the point `lookahead` in front of the robot
// heading, segment = 0. One finite point, or no segment of at least 1e-6 m in
// the search range: rule 2 from point `segment`, which is left unchanged. A
// `segment` that is not the start of a segment of this path is treated as the
// first finite point. A zero or negative `window` makes 1b take the nearest
// segment of the whole plan whenever it is nearer; a NaN `window` disables 1b.
// path and robot are in one frame; the caller checks that.
compass::Point2D localGoalAhead(
  const compass::SE2 & robot,
  const std::vector<compass::Point2D> & path,
  double lookahead,
  std::size_t & segment,
  double window = kPlanSearchWindow);

}  // namespace compass_nav2

#endif  // COMPASS_NAV2__PATH_TRACKING_HPP_
