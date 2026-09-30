#include <gtest/gtest.h>
#include "compass_nav2/compass_controller.hpp"
class ProgressProbe : public compass_nav2::CompassController {
 public:
  void measured() {use_measured_progress_=true;}
  void seed() {state_.L_real=.4;state_.rho=.4;}
  double progress() const {return state_.L_real;}
  void candidateOnly() {
    use_candidate_trajectories_=true;
    use_measured_progress_=false;
    global_frame_="odom";
    clock_=std::make_shared<rclcpp::Clock>(RCL_SYSTEM_TIME);
    core_=std::make_unique<compass::DecisionCore>(knobs_);
  }
  int interventions() const {return state_.n_thrash;}
  void futureSafetyClock() {
    state_.n_thrash=1;
    state_.safe_switch_times={clock_->now().seconds()+1000};
  }
  void legacy() { candidateOnly(); use_candidate_trajectories_=false; }
};
TEST(PlanProgress, RepublishPreservesButChangedGeometryResets) {
  ProgressProbe controller;controller.measured();
  nav_msgs::msg::Path path;path.header.frame_id="odom";path.poses.resize(2);
  path.poses[1].pose.position.x=4;
  controller.setPlan(path);controller.seed();
  path.header.stamp.sec=10;path.poses[0].header.stamp.sec=10;
  controller.setPlan(path);EXPECT_DOUBLE_EQ(controller.progress(),.4);
  path.poses[1].pose.position.y=.5;
  controller.setPlan(path);EXPECT_DOUBLE_EQ(controller.progress(),0);
  controller.seed();path.header.frame_id="map";
  controller.setPlan(path);EXPECT_DOUBLE_EQ(controller.progress(),0);
}

TEST(PlanProgress, LegacyAndCandidateRejectFrameMismatchBeforeDecision) {
  for (int fixture : {0, 1, 2, 3}) {
    const bool mismatch_pose=fixture%2;
    ProgressProbe controller;
    if (fixture<2) controller.candidateOnly(); else controller.legacy();
    nav_msgs::msg::Path path;
    path.header.frame_id=mismatch_pose ? "odom" : "map";
    path.poses.resize(2);
    path.poses[1].pose.position.x=4;
    controller.setPlan(path);
    controller.seed();
    geometry_msgs::msg::PoseStamped pose;
    pose.header.frame_id=mismatch_pose ? "map" : "odom";
    pose.pose.orientation.w=1;
    const auto cmd=controller.computeVelocityCommands(pose, {}, nullptr);
    EXPECT_DOUBLE_EQ(cmd.twist.linear.x,0);
    EXPECT_DOUBLE_EQ(cmd.twist.angular.z,0);
    // A missing costmap would independently yield zero after entering safety;
    // this verifies the frame guard runs before touching decision state.
    EXPECT_EQ(controller.interventions(),0);
    EXPECT_DOUBLE_EQ(controller.progress(),.4);
  }
}

TEST(PlanProgress, CoreContractViolationReturnsZeroWithoutEscapingAdapter) {
  ProgressProbe controller;
  controller.candidateOnly();
  nav_msgs::msg::Path path;
  path.header.frame_id="odom";
  path.poses.resize(2);
  path.poses[1].pose.position.x=4;
  controller.setPlan(path);
  controller.futureSafetyClock();
  geometry_msgs::msg::PoseStamped pose;
  pose.header.frame_id="odom";
  pose.pose.orientation.w=1;
  geometry_msgs::msg::TwistStamped cmd;
  EXPECT_NO_THROW(cmd=controller.computeVelocityCommands(pose, {}, nullptr));
  EXPECT_DOUBLE_EQ(cmd.twist.linear.x,0);
  EXPECT_DOUBLE_EQ(cmd.twist.angular.z,0);
  EXPECT_EQ(controller.interventions(),1);
}

TEST(PlanProgress, LegacyMissingMapEmitsZeroInsteadOfCruiseOrBrakingMotion) {
  ProgressProbe controller;
  controller.legacy();
  geometry_msgs::msg::PoseStamped pose;
  pose.header.frame_id="odom";
  pose.pose.orientation.w=1;
  geometry_msgs::msg::Twist velocity;
  velocity.linear.x=.45;
  const auto cmd=controller.computeVelocityCommands(pose, velocity, nullptr);
  EXPECT_DOUBLE_EQ(cmd.twist.linear.x,0);
  EXPECT_DOUBLE_EQ(cmd.twist.angular.z,0);
}
