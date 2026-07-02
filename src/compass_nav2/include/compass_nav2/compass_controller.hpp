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

#include "compass_msgs/msg/people.hpp"

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

  // 최신 /people 메시지를 costmap global_frame 기준 compass::Person 목록으로
  // 환산 (트래커 미수신 시 빈 목록). 변환은 toPersons 자유 함수에 위임한다.
  std::vector<compass::Person> extractPeople(const compass::SE2 & robot) const;

  // /people 구독 콜백 — 최신 메시지를 뮤텍스 보호 하에 저장.
  void peopleCallback(const compass_msgs::msg::People::SharedPtr msg);

  rclcpp_lifecycle::LifecycleNode::WeakPtr node_;
  std::shared_ptr<tf2_ros::Buffer> tf_;
  std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap_ros_;
  rclcpp::Logger logger_{rclcpp::get_logger("CompassController")};
  rclcpp::Clock::SharedPtr clock_;
  std::string plugin_name_;
  std::string base_frame_;

  nav_msgs::msg::Path global_plan_;

  // /people 구독 + 최신 메시지 (people_mutex_ 보호).
  rclcpp::Subscription<compass_msgs::msg::People>::SharedPtr people_sub_;
  compass_msgs::msg::People::SharedPtr latest_people_;
  std::string global_frame_;       // costmap global_frame (변환 대상 프레임).
  mutable std::mutex people_mutex_;

  // 결정 계층 상태 (주기 간 보존).
  compass::Knobs knobs_;
  std::unique_ptr<compass::DecisionCore> core_;
  compass::DecisionState state_;
  CostmapEnvQuery env_;

  // 속도 한계 (setSpeedLimit).
  double speed_limit_{0.0};        // m/s; 0 == 무제한
  bool speed_limit_is_pct_{false};
  double max_linear_speed_{0.5};   // m/s; 하드 상한 (hard cap)

  // 궤적 계층 ②: 경로 추종 cruise (결정 코어가 정지 상태에서 전진 속도를
  // 산출하지 못하므로 플러그인이 부트스트랩한다).
  double cruise_speed_{0.45};      // m/s; NORMAL 기본 순항 속도
  double max_angular_speed_{1.0};  // rad/s; yaw rate 상한
  double goal_decel_dist_{0.6};    // m; 목표 근처 감속 시작 거리

  // 조향(경로 추종 cross-track + heading PD) 노브. lookahead_dist_ 는 결정
  // 코어용 로컬 목표 거리이고, k_e/k_theta/k_side 는 pathTrackingAngularZ 의
  // 횡오차·헤딩·측면 편향 이득이다 (과감쇠 기본값, 직선에서 위빙 없음).
  double lookahead_dist_{1.0};     // m; 로컬 목표(코어 입력) lookahead 거리
  double k_e_{3.0};                // cross-track 이득
  double k_theta_{3.0};            // heading 이득
  double k_side_{0.4};             // 사회적 측면 편향 이득
  double last_now_{0.0};
  bool has_last_now_{false};

  std::mutex mutex_;
};

}  // namespace compass_nav2

#endif  // COMPASS_NAV2__COMPASS_CONTROLLER_HPP_
