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

// Deployment behaviour of the configured plugin (real configure() on a lifecycle
// node with a configured costmap): people topic, people freshness, operator
// diagnostics and the Humble task boundary.

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "compass_nav2/compass_controller.hpp"

namespace
{
class RuntimeProbe : public compass_nav2::CompassController
{
public:
  std::string peopleTopic() const {return people_sub_->get_topic_name();}
  // Drive the monotonic clock from the test.
  void useSteady(const double * t) {steady_now_ = [t] {return *t;};}
  void deliver(const compass_msgs::msg::People & msg)
  {
    peopleCallback(std::make_shared<compass_msgs::msg::People>(msg));
  }
  compass::Mode mode() const {return state_.mode;}
  uint64_t tfFailures() const {return tf_failures_.load();}
};

// A straight 4 m path along +x in the costmap frame ("map"), robot at its start.
nav_msgs::msg::Path straightPath()
{
  nav_msgs::msg::Path path;
  path.header.frame_id = "map";
  for (int i = 0; i <= 40; ++i) {
    geometry_msgs::msg::PoseStamped p;
    p.header.frame_id = "map";
    p.pose.position.x = 0.5 + 0.1 * i;
    p.pose.position.y = 2.5;
    p.pose.orientation.w = 1.0;
    path.poses.push_back(p);
  }
  return path;
}

geometry_msgs::msg::PoseStamped startPose()
{
  geometry_msgs::msg::PoseStamped pose;
  pose.header.frame_id = "map";
  pose.pose.position.x = 0.5;
  pose.pose.position.y = 2.5;
  pose.pose.orientation.w = 1.0;
  return pose;
}

// One person well off the path (no stop), in the costmap frame.
compass_msgs::msg::People onePerson(double y = 4.0)
{
  compass_msgs::msg::People msg;
  msg.header.frame_id = "map";
  compass_msgs::msg::Person p;
  p.id = 7;
  p.x = 3.0;
  p.y = y;
  msg.people.push_back(p);
  return msg;
}

struct Fixture
{
  rclcpp_lifecycle::LifecycleNode::SharedPtr node;
  std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap;
  std::shared_ptr<tf2_ros::Buffer> tf;

  explicit Fixture(
    const std::vector<rclcpp::Parameter> & overrides = {}, const std::string & ns = "/")
  {
    rclcpp::NodeOptions options;
    options.parameter_overrides(overrides);
    node = std::make_shared<rclcpp_lifecycle::LifecycleNode>("controller_server", ns, options);
    costmap = std::make_shared<nav2_costmap_2d::Costmap2DROS>("runtime_probe_costmap");
    costmap->configure();
    tf = std::make_shared<tf2_ros::Buffer>(node->get_clock());
  }
  ~Fixture() {costmap->cleanup();}

  void configure(compass_nav2::CompassController & c)
  {
    c.configure(node, "FollowPath", tf, costmap);
  }
};
}  // namespace

TEST(PeopleTopic, DefaultIsTheAbsolutePeopleTopic)
{
  Fixture f({}, "/robot1");
  RuntimeProbe controller;
  f.configure(controller);
  EXPECT_EQ(controller.peopleTopic(), "/people");
  controller.cleanup();
}

TEST(PeopleTopic, RelativeNameResolvesInTheNodeNamespace)
{
  Fixture f({{"FollowPath.people_topic", "tracked_people"}}, "/robot1");
  RuntimeProbe controller;
  f.configure(controller);
  EXPECT_EQ(controller.peopleTopic(), "/robot1/tracked_people");
  controller.cleanup();
}

TEST(PeopleFreshness, AssessUsesReceiptAgeAndStampAge)
{
  using compass_nav2::assessPeople;
  EXPECT_TRUE(assessPeople(false, 0, 10, 0, 0, 0.5).stale);
  EXPECT_FALSE(assessPeople(true, 10.0, 10.2, 0, 0, 0.5).stale);
  EXPECT_TRUE(assessPeople(true, 10.0, 10.6, 0, 0, 0.5).stale);
  // Fresh receipt of an old stamp is stale; a stamp ahead of the clock is ignored.
  const auto old_stamp = assessPeople(true, 10.0, 10.0, 100.0, 101.0, 0.5);
  EXPECT_TRUE(old_stamp.stale);
  EXPECT_DOUBLE_EQ(old_stamp.age_s, 1.0);
  EXPECT_FALSE(assessPeople(true, 10.0, 10.0, 105.0, 101.0, 0.5).stale);
}

// With the default "warn", a stale people message changes nothing in the command:
// the first command from identical inputs is the same whether the message is
// 0.1 s or 10 s old.
TEST(PeopleFreshness, DefaultWarnLeavesTheCommandUnchanged)
{
  geometry_msgs::msg::TwistStamped cmd[2];
  for (int stale = 0; stale < 2; ++stale) {
    Fixture f;
    RuntimeProbe controller;
    double t = 100.0;
    controller.useSteady(&t);
    f.configure(controller);
    controller.setPlan(straightPath());
    controller.deliver(onePerson());
    t += stale ? 10.0 : 0.1;
    cmd[stale] = controller.computeVelocityCommands(startPose(), geometry_msgs::msg::Twist(), nullptr);
    controller.cleanup();
  }
  EXPECT_GT(cmd[0].twist.linear.x, 0.0);  // a non-trivial command to compare
  EXPECT_DOUBLE_EQ(cmd[1].twist.linear.x, cmd[0].twist.linear.x);
  EXPECT_DOUBLE_EQ(cmd[1].twist.angular.z, cmd[0].twist.angular.z);
}

TEST(PeopleFreshness, HoldEmitsZeroOnlyWhileStale)
{
  Fixture f({{"FollowPath.people_stale_action", "hold"}});
  RuntimeProbe controller;
  double t = 100.0;
  controller.useSteady(&t);
  f.configure(controller);
  controller.setPlan(straightPath());
  // Never received: hold.
  auto cmd = controller.computeVelocityCommands(startPose(), geometry_msgs::msg::Twist(), nullptr);
  EXPECT_DOUBLE_EQ(cmd.twist.linear.x, 0.0);
  controller.deliver(onePerson());
  t += 0.1;
  cmd = controller.computeVelocityCommands(startPose(), geometry_msgs::msg::Twist(), nullptr);
  EXPECT_GT(cmd.twist.linear.x, 0.0);
  t += 1.0;  // the tracker stopped publishing
  cmd = controller.computeVelocityCommands(startPose(), geometry_msgs::msg::Twist(), nullptr);
  EXPECT_DOUBLE_EQ(cmd.twist.linear.x, 0.0);
  EXPECT_DOUBLE_EQ(cmd.twist.angular.z, 0.0);
  controller.cleanup();
}

TEST(PeopleFreshness, RejectsUnknownActionAndNonPositiveTimeout)
{
  {
    Fixture f({{"FollowPath.people_stale_action", "stop"}});
    RuntimeProbe controller;
    EXPECT_THROW(f.configure(controller), std::invalid_argument);
  }
  {
    Fixture f({{"FollowPath.people_timeout_s", 0.0}});
    RuntimeProbe controller;
    EXPECT_THROW(f.configure(controller), std::invalid_argument);
  }
}

TEST(PeopleFreshness, TransformFailureIsCounted)
{
  Fixture f;
  RuntimeProbe controller;
  f.configure(controller);
  controller.setPlan(straightPath());
  auto msg = onePerson();
  msg.header.frame_id = "tracker_frame";  // no TF to "map"
  controller.deliver(msg);
  controller.computeVelocityCommands(startPose(), geometry_msgs::msg::Twist(), nullptr);
  controller.computeVelocityCommands(startPose(), geometry_msgs::msg::Twist(), nullptr);
  EXPECT_EQ(controller.tfFailures(), 2u);
  controller.cleanup();
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  rclcpp::init(argc, argv);
  const int rc = RUN_ALL_TESTS();
  rclcpp::shutdown();
  return rc;
}
