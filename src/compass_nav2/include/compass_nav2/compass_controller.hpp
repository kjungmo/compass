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

#ifndef COMPASS_NAV2__COMPASS_CONTROLLER_HPP_
#define COMPASS_NAV2__COMPASS_CONTROLLER_HPP_

#include <memory>
#include <mutex>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "nav2_core/controller.hpp"
#include "nav2_costmap_2d/costmap_2d_ros.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "nav_msgs/msg/path.hpp"
#include "tf2_ros/buffer.h"

#include "compass_core/decision_core.hpp"
#include "compass_core/decision_state.hpp"
#include "compass_core/knobs.hpp"
#include "compass_nav2/costmap_env_query.hpp"

namespace compass_nav2
{

// Commit-and-Cross 결정 코어를 감싸는 nav2_core::Controller 플러그인.
//
// 매 주기 computeVelocityCommands 에서: 현재 자세·속도·로컬 목표·사람(costmap
// 동적 클러스터)으로 compass::DecisionInput 을 만들고, CostmapEnvQuery 를 통해
// DecisionCore::step 을 호출하여 주기 간 DecisionState 를 보존한다. DecisionOutput
// (v_target·mode)을 TwistStamped 로 사상한다 (STOP/HOLD -> 영 twist, 그 외 전진).
class CompassController : public nav2_core::Controller
{
public:
  CompassController() = default;
  ~CompassController() override = default;

  void configure(
    const rclcpp_lifecycle::LifecycleNode::WeakPtr & parent,
    std::string name, std::shared_ptr<tf2_ros::Buffer> tf,
    std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap_ros) override;

  void cleanup() override;
  void activate() override;
  void deactivate() override;
  void reset() override;

  void setPlan(const nav_msgs::msg::Path & path) override;

  geometry_msgs::msg::TwistStamped computeVelocityCommands(
    const geometry_msgs::msg::PoseStamped & pose,
    const geometry_msgs::msg::Twist & velocity,
    nav2_core::GoalChecker * goal_checker) override;

  void setSpeedLimit(const double & speed_limit, const bool & percentage) override;

protected:
  // ROS 파라미터 -> compass::Knobs 로딩 (모든 노브 1:1, plugin-name 스코프).
  void loadKnobs(
    const rclcpp_lifecycle::LifecycleNode::SharedPtr & node, const std::string & name);

  // 전역 계획의 마지막 점(또는 robot 전방 lookahead)을 로컬 목표로 환산.
  compass::Point2D computeLocalGoal(const compass::SE2 & robot) const;

  // costmap 동적 클러스터를 사람 후보로 환산 (단순 근사; 트래커 부재 시 빈 목록).
  std::vector<compass::Person> extractPeople(const compass::SE2 & robot) const;

  rclcpp_lifecycle::LifecycleNode::WeakPtr node_;
  std::shared_ptr<tf2_ros::Buffer> tf_;
  std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap_ros_;
  rclcpp::Logger logger_{rclcpp::get_logger("CompassController")};
  rclcpp::Clock::SharedPtr clock_;
  std::string plugin_name_;
  std::string base_frame_;

  nav_msgs::msg::Path global_plan_;

  // 결정 계층 상태 (주기 간 보존).
  compass::Knobs knobs_;
  std::unique_ptr<compass::DecisionCore> core_;
  compass::DecisionState state_;
  CostmapEnvQuery env_;

  // 속도 한계 (setSpeedLimit).
  double speed_limit_{0.0};        // m/s; 0 == 무제한
  bool speed_limit_is_pct_{false};
  double max_linear_speed_{0.5};   // m/s; v_target 상한
  double last_now_{0.0};
  bool has_last_now_{false};

  std::mutex mutex_;
};

}  // namespace compass_nav2

#endif  // COMPASS_NAV2__COMPASS_CONTROLLER_HPP_
