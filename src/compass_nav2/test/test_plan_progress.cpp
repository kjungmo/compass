#include <gtest/gtest.h>
#include "compass_nav2/compass_controller.hpp"
class ProgressProbe : public compass_nav2::CompassController {
 public:
  void measured() {use_measured_progress_=true;}
  void seed() {state_.L_real=.4;state_.rho=.4;}
  double progress() const {return state_.L_real;}
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
