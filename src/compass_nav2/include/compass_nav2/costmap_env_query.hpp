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

#ifndef COMPASS_NAV2__COSTMAP_ENV_QUERY_HPP_
#define COMPASS_NAV2__COSTMAP_ENV_QUERY_HPP_

#include "compass_core/env_query.hpp"
#include "compass_core/types.hpp"
#include "nav2_costmap_2d/costmap_2d.hpp"

namespace compass_nav2
{

// compass::IEnvQuery 를 nav2_costmap_2d 위에서 구현한다 (research_spec §5.2).
//
// 한 결정 주기 동안 컨트롤러가 현재 costmap·로봇 자세·로컬 목표를 주입하면
// (setContext), 각 class c 에 대해 통과 통로 폭·clearance·TTC·실현 가능성을
// 답한다. 보존적인 근사를 채택한다: clearance 는 로봇 자세에서 로컬 목표 방향
// 으로 진행하면서 만나는 lethal cell 까지의 거리, corridor_width 는 그 진행
// 직교 방향으로 양측 lethal cell 사이 폭, TTC 는 사람 상대 접근에서 산출한다.
// costmap 이 없거나 표본이 비면 안전 측(통과 가능)으로 답한다 — 플러그인은
// 항상 명령을 산출해야 한다.
class CostmapEnvQuery : public compass::IEnvQuery
{
public:
  // 주기 컨텍스트 주입. costmap 은 컨트롤러가 소유; 이 객체는 빌려 본다.
  void setContext(
    const nav2_costmap_2d::Costmap2D * costmap,
    const compass::SE2 & robot_pose,
    const compass::Point2D & local_goal,
    const std::vector<compass::Person> & people,
    const compass::Twist2D & robot_vel);

  double corridor_width(const compass::TopoClass & c) const override;
  double clearance(const compass::TopoClass & c) const override;
  double ttc(const compass::TopoClass & c) const override;
  bool feasible(const compass::TopoClass & c) const override;

private:
  // class 의 측면 부호 합 (L=-1, R=+1)의 평균 -> 횡 오프셋 방향. 빈 class 면 0.
  double lateral_bias(const compass::TopoClass & c) const;
  // (wx,wy) 가 lethal/inscribed 이상으로 점유되었는지.
  bool occupied(double wx, double wy) const;

  const nav2_costmap_2d::Costmap2D * costmap_ = nullptr;
  compass::SE2 robot_pose_;
  compass::Point2D local_goal_;
  std::vector<compass::Person> people_;
  compass::Twist2D robot_vel_;

  // 보존적 기본값 (costmap 부재 시).
  static constexpr double kDefaultClearance = 5.0;     // m
  static constexpr double kDefaultCorridor = 3.0;      // m
  static constexpr double kDefaultTtc = 10.0;          // s
  static constexpr double kRayMax = 4.0;               // m, 전방 ray 길이 상한
  static constexpr double kLateralStep = 0.25;         // m, class 부호당 횡 오프셋
};

}  // namespace compass_nav2

#endif  // COMPASS_NAV2__COSTMAP_ENV_QUERY_HPP_
