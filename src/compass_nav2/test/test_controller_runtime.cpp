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

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "compass_nav2/compass_controller.hpp"
#include "compass_nav2/param_checks.hpp"

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
  using compass_nav2::CompassController::buildDiagnostics;
  void forceHold() {state_.mode = compass::Mode::HOLD;}
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

using Params = std::vector<rclcpp::Parameter>;

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
  Fixture f(Params{}, "/robot1");
  RuntimeProbe controller;
  f.configure(controller);
  EXPECT_EQ(controller.peopleTopic(), "/people");
  controller.cleanup();
}

TEST(PeopleTopic, RelativeNameResolvesInTheNodeNamespace)
{
  Fixture f(Params{{"FollowPath.people_topic", "tracked_people"}}, "/robot1");
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
  Fixture f(Params{{"FollowPath.people_stale_action", "hold"}});
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
    Fixture f(Params{{"FollowPath.people_stale_action", "stop"}});
    RuntimeProbe controller;
    EXPECT_THROW(f.configure(controller), std::invalid_argument);
  }
  {
    Fixture f(Params{{"FollowPath.people_timeout_s", 0.0}});
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

std::string value(const diagnostic_msgs::msg::DiagnosticStatus & st, const std::string & key)
{
  for (const auto & kv : st.values) {
    if (kv.key == key) {return kv.value;}
  }
  return "<missing " + key + ">";
}

// A person 0.7 m ahead walking straight at the moving robot.
compass_msgs::msg::People oncoming()
{
  compass_msgs::msg::People msg;
  msg.header.frame_id = "map";
  compass_msgs::msg::Person p;
  p.id = 3;
  p.x = 1.2;
  p.y = 2.5;
  p.vx = -1.0;
  msg.people.push_back(p);
  return msg;
}

geometry_msgs::msg::Twist moving()
{
  geometry_msgs::msg::Twist v;
  v.linear.x = 0.45;
  return v;
}

TEST(Diagnostics, NormalWithFreshPeopleIsOk)
{
  Fixture f;
  RuntimeProbe controller;
  double t = 100.0;
  controller.useSteady(&t);
  f.configure(controller);
  EXPECT_EQ(value(controller.buildDiagnostics(), "seconds_since_compute"), "never");
  controller.setPlan(straightPath());
  controller.deliver(onePerson());
  t += 0.1;
  controller.computeVelocityCommands(startPose(), geometry_msgs::msg::Twist(), nullptr);
  t += 0.2;
  const auto st = controller.buildDiagnostics();
  EXPECT_EQ(st.name, "controller_server: compass");
  EXPECT_EQ(st.hardware_id, "/");
  EXPECT_EQ(st.level, diagnostic_msgs::msg::DiagnosticStatus::OK) << st.message;
  EXPECT_EQ(value(st, "mode"), "NORMAL");
  EXPECT_EQ(value(st, "people_count"), "1");
  EXPECT_EQ(value(st, "people_applied_last_cycle"), "1");
  EXPECT_EQ(value(st, "people_age_s"), "0.30");
  EXPECT_EQ(value(st, "seconds_since_compute"), "0.20");
  EXPECT_EQ(value(st, "last_command").rfind("drive (v=0.45", 0), 0u) << value(st, "last_command");
  controller.cleanup();
}

TEST(Diagnostics, StaleOrMissingPeopleWarn)
{
  Fixture f;
  RuntimeProbe controller;
  double t = 100.0;
  controller.useSteady(&t);
  f.configure(controller);
  auto st = controller.buildDiagnostics();
  EXPECT_EQ(st.level, diagnostic_msgs::msg::DiagnosticStatus::WARN);
  EXPECT_EQ(st.message, "no people message received on /people");
  controller.deliver(onePerson());
  t += 2.0;  // the tracker stopped; no control call needed for the warning
  st = controller.buildDiagnostics();
  EXPECT_EQ(st.level, diagnostic_msgs::msg::DiagnosticStatus::WARN);
  EXPECT_EQ(st.message, "people input stale (2.00 s)");
  EXPECT_EQ(value(st, "people_stale"), "true");
  controller.cleanup();
}

TEST(Diagnostics, StopIsVisibleAndLatches)
{
  Fixture f;
  RuntimeProbe controller;
  double t = 100.0;
  controller.useSteady(&t);
  f.configure(controller);
  controller.setPlan(straightPath());
  controller.deliver(oncoming());
  const auto cmd = controller.computeVelocityCommands(startPose(), moving(), nullptr);
  ASSERT_EQ(controller.mode(), compass::Mode::STOP);
  EXPECT_DOUBLE_EQ(cmd.twist.linear.x, 0.0);
  auto st = controller.buildDiagnostics();
  EXPECT_EQ(st.level, diagnostic_msgs::msg::DiagnosticStatus::WARN);
  EXPECT_EQ(value(st, "mode"), "STOP");
  EXPECT_NE(st.message.find("STOP latched"), std::string::npos) << st.message;
  EXPECT_EQ(value(st, "last_command").rfind("mode STOP", 0), 0u);
  // The person leaves; published behaviour keeps STOP until reset.
  controller.deliver(onePerson());
  controller.computeVelocityCommands(startPose(), geometry_msgs::msg::Twist(), nullptr);
  EXPECT_EQ(controller.mode(), compass::Mode::STOP);
  controller.reset();
  EXPECT_EQ(value(controller.buildDiagnostics(), "mode"), "NORMAL");
  controller.cleanup();
}

TEST(Diagnostics, PublishedOnlyWhileActive)
{
  Fixture f(Params{{"FollowPath.diagnostics_period_s", 0.1}});
  RuntimeProbe controller;
  f.configure(controller);
  auto listener = std::make_shared<rclcpp::Node>("diagnostics_listener");
  std::atomic<int> received{0};
  auto sub = listener->create_subscription<diagnostic_msgs::msg::DiagnosticArray>(
    "/diagnostics", 10, [&](diagnostic_msgs::msg::DiagnosticArray::SharedPtr m) {
      for (const auto & s : m->status) {
        if (s.name == "controller_server: compass") {++received;}
      }
    });
  rclcpp::executors::SingleThreadedExecutor exec;
  exec.add_node(f.node->get_node_base_interface());
  exec.add_node(listener);
  const auto spin_for = [&](std::chrono::milliseconds d) {
      const auto end = std::chrono::steady_clock::now() + d;
      while (std::chrono::steady_clock::now() < end) {exec.spin_some(std::chrono::milliseconds(20));}
    };
  spin_for(std::chrono::milliseconds(500));
  EXPECT_EQ(received.load(), 0);  // configured, not active
  controller.activate();
  for (int i = 0; i < 100 && received.load() < 3; ++i) {spin_for(std::chrono::milliseconds(50));}
  EXPECT_GE(received.load(), 3);
  controller.deactivate();
  spin_for(std::chrono::milliseconds(300));  // drain messages already in flight
  const int after = received.load();
  spin_for(std::chrono::milliseconds(700));
  EXPECT_EQ(received.load(), after);
  controller.cleanup();
}

TEST(TaskBoundary, GapRule)
{
  using compass_nav2::controlGapStartsNewTask;
  EXPECT_FALSE(controlGapStartsNewTask(false, 0.0, 50.0, 1.0));  // first call
  EXPECT_FALSE(controlGapStartsNewTask(true, 10.0, 10.05, 1.0));  // next period
  EXPECT_FALSE(controlGapStartsNewTask(true, 10.0, 11.0, 1.0));   // boundary excluded
  EXPECT_TRUE(controlGapStartsNewTask(true, 10.0, 11.01, 1.0));
  EXPECT_FALSE(controlGapStartsNewTask(true, 10.0, 99.0, 0.0));   // disabled
}

// Humble (no reset hook): a control-call gap above task_gap_reset_s starts a new
// task, as Jazzy's reset() at task end does. Jazzy: the gap changes nothing.
TEST(TaskBoundary, ControlGapResetsOnlyWithoutResetHook)
{
  Fixture f;
  RuntimeProbe controller;
  double t = 100.0;
  controller.useSteady(&t);
  f.configure(controller);
  controller.setPlan(straightPath());
  controller.deliver(oncoming());
  controller.computeVelocityCommands(startPose(), moving(), nullptr);
  ASSERT_EQ(controller.mode(), compass::Mode::STOP);
  controller.deliver(onePerson());
  t += 0.5;  // within a task
  controller.computeVelocityCommands(startPose(), geometry_msgs::msg::Twist(), nullptr);
  EXPECT_EQ(controller.mode(), compass::Mode::STOP);
  t += 2.0;  // the next goal starts after a 2 s pause
  controller.deliver(onePerson());
  const auto cmd =
    controller.computeVelocityCommands(startPose(), geometry_msgs::msg::Twist(), nullptr);
#if defined(COMPASS_NAV2_CONTROLLER_HAS_RESET) && !COMPASS_NAV2_CONTROLLER_HAS_RESET
  EXPECT_EQ(controller.mode(), compass::Mode::NORMAL);
  EXPECT_GT(cmd.twist.linear.x, 0.0);
#else
  EXPECT_EQ(controller.mode(), compass::Mode::STOP);
  EXPECT_DOUBLE_EQ(cmd.twist.linear.x, 0.0);
#endif
  controller.cleanup();
}

// ---- Opt-in STOP/HOLD release (beyond the published method; off by default) ----

namespace
{
// Enters STOP at t, then the person walks away; returns the controller in STOP.
void enterStop(RuntimeProbe & controller, double & t)
{
  controller.setPlan(straightPath());
  controller.deliver(oncoming());
  controller.computeVelocityCommands(startPose(), moving(), nullptr);
  ASSERT_EQ(controller.mode(), compass::Mode::STOP);
  (void)t;
}

// One control call dt later; the tracker reports nobody (the person has left),
// so the committed empty class is safe and the core takes no safety branch.
geometry_msgs::msg::TwistStamped step(RuntimeProbe & controller, double & t, double dt)
{
  t += dt;
  compass_msgs::msg::People nobody;
  nobody.header.frame_id = "map";
  controller.deliver(nobody);  // fresh input
  return controller.computeVelocityCommands(startPose(), geometry_msgs::msg::Twist(), nullptr);
}
}  // namespace

TEST(OptionalRelease, DefaultsLatchStopAndHoldAsPublished)
{
  for (const auto & params : {Params{}, Params{{"FollowPath.stop_release_dwell_s", -1.0},
      {"FollowPath.hold_release_after_s", -1.0}}})
  {
    Fixture f(params);
    RuntimeProbe controller;
    double t = 100.0;
    controller.useSteady(&t);
    f.configure(controller);
    enterStop(controller, t);
    for (int i = 0; i < 60; ++i) {  // 30 s with the person gone
      const auto cmd = step(controller, t, 0.5);
      EXPECT_EQ(controller.mode(), compass::Mode::STOP);
      EXPECT_DOUBLE_EQ(cmd.twist.linear.x, 0.0);
      EXPECT_DOUBLE_EQ(cmd.twist.angular.z, 0.0);
    }
    controller.forceHold();
    for (int i = 0; i < 60; ++i) {
      EXPECT_DOUBLE_EQ(step(controller, t, 0.5).twist.linear.x, 0.0);
      EXPECT_EQ(controller.mode(), compass::Mode::HOLD);
    }
    controller.cleanup();
  }
}

TEST(OptionalRelease, StopReleasesAfterContinuousSafeDwell)
{
  Fixture f(Params{{"FollowPath.stop_release_dwell_s", 0.5}});
  RuntimeProbe controller;
  double t = 100.0;
  controller.useSteady(&t);
  f.configure(controller);
  enterStop(controller, t);
  step(controller, t, 0.1);  // TTC clear from here (t = 100.1)
  EXPECT_EQ(controller.mode(), compass::Mode::STOP);
  step(controller, t, 0.3);  // 0.3 s clear
  EXPECT_EQ(controller.mode(), compass::Mode::STOP);
  auto cmd = step(controller, t, 0.25);  // 0.55 s clear: released, this command still zero
  EXPECT_EQ(controller.mode(), compass::Mode::NORMAL);
  EXPECT_DOUBLE_EQ(cmd.twist.linear.x, 0.0);
  cmd = step(controller, t, 0.05);
  EXPECT_GT(cmd.twist.linear.x, 0.0);
  controller.cleanup();
}

TEST(OptionalRelease, StopDwellRestartsWhenDangerReturns)
{
  Fixture f(Params{{"FollowPath.stop_release_dwell_s", 0.5}});
  RuntimeProbe controller;
  double t = 100.0;
  controller.useSteady(&t);
  f.configure(controller);
  enterStop(controller, t);
  step(controller, t, 0.1);  // clear since 100.1
  step(controller, t, 0.3);
  t += 0.05;  // the person comes back: TTC < ttc_stop
  controller.deliver(oncoming());
  controller.computeVelocityCommands(startPose(), moving(), nullptr);
  EXPECT_EQ(controller.mode(), compass::Mode::STOP);
  step(controller, t, 0.1);  // clear again since here
  step(controller, t, 0.3);
  EXPECT_EQ(controller.mode(), compass::Mode::STOP);  // only 0.3 s of the new dwell
  step(controller, t, 0.25);
  EXPECT_EQ(controller.mode(), compass::Mode::NORMAL);
  controller.cleanup();
}

TEST(OptionalRelease, HoldReleasesAfterConfiguredTime)
{
  Fixture f(Params{{"FollowPath.hold_release_after_s", 2.0}});
  RuntimeProbe controller;
  double t = 100.0;
  controller.useSteady(&t);
  f.configure(controller);
  controller.setPlan(straightPath());
  controller.forceHold();
  EXPECT_DOUBLE_EQ(step(controller, t, 0.05).twist.linear.x, 0.0);  // HOLD since 100.05
  EXPECT_DOUBLE_EQ(step(controller, t, 1.5).twist.linear.x, 0.0);
  EXPECT_EQ(controller.mode(), compass::Mode::HOLD);
  const auto cmd = step(controller, t, 0.6);  // 2.1 s: released before deciding
  EXPECT_EQ(controller.mode(), compass::Mode::NORMAL);
  EXPECT_GT(cmd.twist.linear.x, 0.0);
  controller.cleanup();
}

TEST(OptionalRelease, RejectsNegativeDurationsOtherThanMinusOne)
{
  for (const char * key : {"FollowPath.stop_release_dwell_s", "FollowPath.hold_release_after_s"}) {
    Fixture f(Params{{key, -0.5}});
    RuntimeProbe controller;
    EXPECT_THROW(f.configure(controller), std::invalid_argument) << key;
  }
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  rclcpp::init(argc, argv);
  const int rc = RUN_ALL_TESTS();
  rclcpp::shutdown();
  return rc;
}
