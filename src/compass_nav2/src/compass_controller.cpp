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

#include "compass_nav2/compass_controller.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "nav2_costmap_2d/cost_values.hpp"
#include "nav2_util/node_utils.hpp"
#include "pluginlib/class_list_macros.hpp"
#include "tf2/utils.hpp"

#include "compass_nav2/people_conversion.hpp"

namespace compass_nav2
{

namespace
{
// 헬퍼: 파라미터 1개를 plugin-name 스코프로 선언 후 읽는다.
template<typename T>
void getParam(
  const rclcpp_lifecycle::LifecycleNode::SharedPtr & node,
  const std::string & ns, const std::string & key, T & out, const T & def)
{
  const std::string full = ns + "." + key;
  nav2_util::declare_parameter_if_not_declared(node, full, rclcpp::ParameterValue(def));
  node->get_parameter(full, out);
}
}  // namespace

void CompassController::configure(
  const rclcpp_lifecycle::LifecycleNode::WeakPtr & parent,
  std::string name, std::shared_ptr<tf2_ros::Buffer> tf,
  std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap_ros)
{
  node_ = parent;
  auto node = parent.lock();
  plugin_name_ = name;
  tf_ = tf;
  costmap_ros_ = costmap_ros;
  logger_ = node->get_logger();
  clock_ = node->get_clock();
  base_frame_ = costmap_ros_->getBaseFrameID();
  global_frame_ = costmap_ros_->getGlobalFrameID();

  loadKnobs(node, name);
  core_ = std::make_unique<compass::DecisionCore>(knobs_);
  state_ = compass::DecisionState{};

  // /people 구독 — 외부 트래커(또는 시뮬 지상 진실)가 보내는 추적 사람 목록.
  // sensor_data QoS(best-effort) 로 콜백에서 최신 메시지만 보관한다.
  people_sub_ = node->create_subscription<compass_msgs::msg::People>(
    "/people", rclcpp::SensorDataQoS(),
    std::bind(&CompassController::peopleCallback, this, std::placeholders::_1));

  RCLCPP_INFO(
    logger_, "CompassController '%s' 구성 완료 (E0=%.3f, e_max_rev=%.3f, max_v=%.3f, "
    "global_frame=%s, /people 구독).",
    name.c_str(), knobs_.E0, knobs_.e_max_rev, max_linear_speed_, global_frame_.c_str());
}

void CompassController::peopleCallback(const compass_msgs::msg::People::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(people_mutex_);
  latest_people_ = msg;
}

void CompassController::loadKnobs(
  const rclcpp_lifecycle::LifecycleNode::SharedPtr & node, const std::string & name)
{
  compass::Knobs k;  // 사양서 기본값에서 출발.
  getParam(node, name, "delta_floor", k.delta_floor, k.delta_floor);
  getParam(node, name, "E0", k.E0, k.E0);
  getParam(node, name, "k_rho", k.k_rho, k.k_rho);
  getParam(node, name, "p", k.p, k.p);
  getParam(node, name, "lambda", k.lambda, k.lambda);
  getParam(node, name, "e_max_fwd", k.e_max_fwd, k.e_max_fwd);
  getParam(node, name, "e_max_rev", k.e_max_rev, k.e_max_rev);
  getParam(node, name, "d_safe", k.d_safe, k.d_safe);
  getParam(node, name, "ttc_min", k.ttc_min, k.ttc_min);
  getParam(node, name, "W", k.W, k.W);
  getParam(node, name, "n_thrash", k.n_thrash, k.n_thrash);
  getParam(node, name, "w_g", k.w_g, k.w_g);
  getParam(node, name, "w_s", k.w_s, k.w_s);
  getParam(node, name, "w_e", k.w_e, k.w_e);
  getParam(node, name, "w_r", k.w_r, k.w_r);
  getParam(node, name, "sigma_front", k.sigma_front, k.sigma_front);
  getParam(node, name, "sigma_rear", k.sigma_rear, k.sigma_rear);
  getParam(node, name, "sigma_s", k.sigma_s, k.sigma_s);
  getParam(node, name, "K_cap", k.K_cap, k.K_cap);
  getParam(node, name, "a_brake", k.a_brake, k.a_brake);
  getParam(node, name, "ttc_stop", k.ttc_stop, k.ttc_stop);
  getParam(node, name, "T_safe_dwell", k.T_safe_dwell, k.T_safe_dwell);
  getParam(node, name, "eps_in", k.eps_in, k.eps_in);
  getParam(node, name, "eps_out", k.eps_out, k.eps_out);
  knobs_ = k;
  getParam(node, name, "max_linear_speed", max_linear_speed_, max_linear_speed_);
}

void CompassController::cleanup()
{
  RCLCPP_INFO(logger_, "CompassController '%s' 정리.", plugin_name_.c_str());
  people_sub_.reset();
  {
    std::lock_guard<std::mutex> lock(people_mutex_);
    latest_people_.reset();
  }
  core_.reset();
  costmap_ros_.reset();
  tf_.reset();
}

void CompassController::activate()
{
  RCLCPP_INFO(logger_, "CompassController '%s' 활성화.", plugin_name_.c_str());
}

void CompassController::deactivate()
{
  RCLCPP_INFO(logger_, "CompassController '%s' 비활성화.", plugin_name_.c_str());
}

void CompassController::reset()
{
  std::lock_guard<std::mutex> lock(mutex_);
  state_ = compass::DecisionState{};
  has_last_now_ = false;
}

void CompassController::setPlan(const nav_msgs::msg::Path & path)
{
  std::lock_guard<std::mutex> lock(mutex_);
  global_plan_ = path;
}

void CompassController::setSpeedLimit(const double & speed_limit, const bool & percentage)
{
  speed_limit_ = speed_limit;
  speed_limit_is_pct_ = percentage;
}

compass::Point2D CompassController::computeLocalGoal(const compass::SE2 & robot) const
{
  // 전역 계획에서 로봇 전방으로 lookahead 거리만큼 떨어진 점을 로컬 목표로 쓴다.
  // 계획이 비면 로봇 헤딩 전방 1 m 를 목표로 둔다.
  constexpr double kLookahead = 1.0;  // m
  if (global_plan_.poses.empty()) {
    compass::Point2D g;
    g.x = robot.x + kLookahead * std::cos(robot.theta);
    g.y = robot.y + kLookahead * std::sin(robot.theta);
    return g;
  }
  // 로봇에서 lookahead 이상 떨어진 첫 점, 없으면 마지막 점.
  for (const auto & ps : global_plan_.poses) {
    const double dx = ps.pose.position.x - robot.x;
    const double dy = ps.pose.position.y - robot.y;
    if (std::hypot(dx, dy) >= kLookahead) {
      return {ps.pose.position.x, ps.pose.position.y};
    }
  }
  const auto & last = global_plan_.poses.back().pose.position;
  return {last.x, last.y};
}

std::vector<compass::Person> CompassController::extractPeople(const compass::SE2 & robot) const
{
  // 최신 /people 메시지를 costmap global_frame 기준 compass::Person 목록으로
  // 변환한다. 메시지가 없으면 빈 목록(보존적) — 컨트롤러는 사람 0명이어도
  // 명령을 산출한다. 변환·프레임 처리는 단위 테스트 가능한 toPersons 에 위임.
  (void)robot;
  compass_msgs::msg::People::SharedPtr msg;
  {
    std::lock_guard<std::mutex> lock(people_mutex_);
    msg = latest_people_;
  }
  if (!msg) {
    return {};
  }
  std::vector<compass::Person> people = toPersons(*msg, global_frame_, tf_);

  // 사람 수를 throttle 로그로 남겨 스모크가 수신을 확인할 수 있게 한다.
  RCLCPP_INFO_THROTTLE(
    logger_, *clock_, 2000, "CompassController: /people 수신 — %zu 명 적용.",
    people.size());

  return people;
}

geometry_msgs::msg::TwistStamped CompassController::computeVelocityCommands(
  const geometry_msgs::msg::PoseStamped & pose,
  const geometry_msgs::msg::Twist & velocity,
  nav2_core::GoalChecker * /*goal_checker*/)
{
  std::lock_guard<std::mutex> lock(mutex_);

  geometry_msgs::msg::TwistStamped cmd;
  cmd.header.frame_id = pose.header.frame_id;
  cmd.header.stamp = clock_ ? clock_->now() : rclcpp::Clock().now();

  // 1) 자세·속도 -> compass 자료형.
  compass::SE2 robot;
  robot.x = pose.pose.position.x;
  robot.y = pose.pose.position.y;
  robot.theta = tf2::getYaw(pose.pose.orientation);

  compass::Twist2D rvel;
  rvel.vx = velocity.linear.x;
  rvel.wz = velocity.angular.z;

  // 2) 결정 주기 입력 조립.
  compass::DecisionInput in;
  in.robot_pose = robot;
  in.robot_vel = rvel;
  in.local_goal = computeLocalGoal(robot);
  in.people = extractPeople(robot);

  const double now = cmd.header.stamp.sec + cmd.header.stamp.nanosec * 1e-9;
  in.now = now;
  // 첫 주기 또는 시계 역행 시 공칭 dt; 그 외엔 실측 주기 간격.
  in.dt = (has_last_now_ && now > last_now_) ? (now - last_now_) : 0.1;
  last_now_ = now;
  has_last_now_ = true;

  // 3) costmap 컨텍스트 주입.
  const nav2_costmap_2d::Costmap2D * costmap =
    costmap_ros_ ? costmap_ros_->getCostmap() : nullptr;
  env_.setContext(costmap, robot, in.local_goal, in.people, rvel);

  // 4) 결정 코어 1주기 — DecisionState 는 멤버로 보존된다.
  compass::DecisionOutput out = core_->step(in, state_, env_);

  // 5) DecisionOutput -> TwistStamped.
  if (out.mode == compass::Mode::STOP || out.mode == compass::Mode::HOLD) {
    cmd.twist.linear.x = 0.0;
    cmd.twist.angular.z = 0.0;
    return cmd;
  }

  // NORMAL: 전진 + 선택된 class 의 측면으로 향하도록 yaw 명령.
  double v = std::max(0.0, out.v_target);
  // 속도 한계 적용.
  double v_cap = max_linear_speed_;
  if (speed_limit_ > 0.0) {
    v_cap = speed_limit_is_pct_ ? max_linear_speed_ * (speed_limit_ / 100.0) : speed_limit_;
  }
  v = std::min(v, v_cap);

  // 로컬 목표 방향과 class 측면을 향하는 보존적 yaw rate.
  const double dgx = in.local_goal.x - robot.x;
  const double dgy = in.local_goal.y - robot.y;
  double yaw_err = std::atan2(dgy, dgx) - robot.theta;
  while (yaw_err > M_PI) {yaw_err -= 2.0 * M_PI;}
  while (yaw_err < -M_PI) {yaw_err += 2.0 * M_PI;}

  // class 의 측면 부호로 약한 횡 편향을 더한다 (R -> 우측, L -> 좌측).
  double side_bias = 0.0;
  for (const auto & [id, s] : out.c_star.pairs()) {
    (void)id;
    side_bias += (s == compass::Side::R) ? -1.0 : 1.0;
  }
  if (out.c_star.size() > 0) {
    side_bias /= static_cast<double>(out.c_star.size());
  }
  constexpr double kYawGain = 1.0;
  constexpr double kSideGain = 0.3;
  cmd.twist.linear.x = v;
  cmd.twist.angular.z = std::clamp(
    kYawGain * yaw_err + kSideGain * side_bias, -knobs_.W, knobs_.W);
  return cmd;
}

}  // namespace compass_nav2

PLUGINLIB_EXPORT_CLASS(compass_nav2::CompassController, nav2_core::Controller)
