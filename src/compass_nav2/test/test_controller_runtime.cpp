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
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "compass_nav2/compass_controller.hpp"
#include "compass_nav2/param_checks.hpp"
#include "compass_nav2/task_boundary.hpp"

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
  const compass_nav2::TaskBoundary & taskBoundary() const {return task_boundary_;}
  double lReal() const {return state_.L_real;}
  // Drive the decision (ROS) clock from the test.
  void useRosTime(double s)
  {
    if (!ros_clock_) {
      ros_clock_ = std::make_shared<rclcpp::Clock>(RCL_ROS_TIME);
      if (rcl_enable_ros_time_override(ros_clock_->get_clock_handle()) != RCL_RET_OK) {
        throw std::runtime_error("cannot enable the ROS time override");
      }
      clock_ = ros_clock_;
    }
    if (rcl_set_ros_time_override(
        ros_clock_->get_clock_handle(), static_cast<int64_t>(s * 1e9)) != RCL_RET_OK)
    {
      throw std::runtime_error("cannot set the ROS time override");
    }
  }
  rclcpp::Clock::SharedPtr ros_clock_;
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

// After a "hold" the first decision uses the published 0.1 s fallback interval,
// not the length of the hold. Observed through the legacy progress proxy:
// L_real grows by |v| * dt per discretionary decision.
TEST(PeopleFreshness, HoldDoesNotInflateTheNextDecisionInterval)
{
  Fixture f(Params{{"FollowPath.people_stale_action", "hold"}});
  RuntimeProbe controller;
  double t = 100.0;
  controller.useSteady(&t);
  f.configure(controller);
  controller.useRosTime(t);
  controller.setPlan(straightPath());
  compass_msgs::msg::People nobody;
  nobody.header.frame_id = "map";
  geometry_msgs::msg::Twist v;
  v.linear.x = 0.45;
  const auto decide = [&](double dt, bool fresh) {
      t += dt;
      controller.useRosTime(t);
      if (fresh) {controller.deliver(nobody);}
      const double before = controller.lReal();
      controller.computeVelocityCommands(startPose(), v, nullptr);
      return controller.lReal() - before;
    };
  EXPECT_NEAR(decide(0.0, true), 0.45 * 0.1, 1e-9);   // first decision: fallback
  EXPECT_NEAR(decide(0.05, true), 0.45 * 0.05, 1e-9);  // measured interval
  EXPECT_DOUBLE_EQ(decide(3.0, false), 0.0);          // stale: hold, no decision
  EXPECT_NEAR(decide(0.05, true), 0.45 * 0.1, 1e-9);  // not 0.45 * 3.05
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

std::string value(const diagnostic_msgs::msg::DiagnosticStatus & st, const std::string & key);

TEST(PeopleFreshness, TransformFailureIsCounted)
{
  Fixture f;
  RuntimeProbe controller;
  double t = 100.0;
  controller.useSteady(&t);
  f.configure(controller);
  controller.setPlan(straightPath());
  auto msg = onePerson();
  msg.header.frame_id = "tracker_frame";  // no TF to "map"
  controller.deliver(msg);
  controller.computeVelocityCommands(startPose(), geometry_msgs::msg::Twist(), nullptr);
  t += 0.05;
  controller.deliver(msg);
  controller.computeVelocityCommands(startPose(), geometry_msgs::msg::Twist(), nullptr);
  EXPECT_EQ(controller.tfFailures(), 2u);
  // Fresh input, but every person dropped: the status must say so (WARN).
  auto st = controller.buildDiagnostics();
  EXPECT_EQ(st.level, diagnostic_msgs::msg::DiagnosticStatus::WARN);
  EXPECT_EQ(st.message, "people TF failing, deciding without people (2 failures)");
  EXPECT_EQ(value(st, "people_tf_last_failure_age_s"), "0.00");
  // TF recovers: the warning clears once the last failure is older than people_timeout_s.
  t += 0.3;
  controller.deliver(onePerson());
  controller.computeVelocityCommands(startPose(), geometry_msgs::msg::Twist(), nullptr);
  EXPECT_EQ(controller.buildDiagnostics().level, diagnostic_msgs::msg::DiagnosticStatus::WARN);
  t += 0.3;
  controller.deliver(onePerson());
  st = controller.buildDiagnostics();
  EXPECT_EQ(st.level, diagnostic_msgs::msg::DiagnosticStatus::OK) << st.message;
  EXPECT_EQ(value(st, "people_tf_failures"), "2");
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
  EXPECT_EQ(st.name, "controller_server: compass (FollowPath)");
  EXPECT_EQ(st.hardware_id, "/");
  EXPECT_EQ(st.level, diagnostic_msgs::msg::DiagnosticStatus::OK) << st.message;
  EXPECT_EQ(st.message, "ok");
  // Every field the deployment notes list, in order.
  std::vector<std::string> keys;
  for (const auto & kv : st.values) {keys.push_back(kv.key);}
  EXPECT_EQ(
    keys, (std::vector<std::string>{"plugin", "mode", "committed_class", "e_rev", "rho",
      "people_topic", "people_count", "people_age_s", "people_timeout_s", "people_stale",
      "people_applied_last_cycle", "people_tf_failures", "people_tf_last_failure_age_s",
      "seconds_since_compute", "last_command", "stop_release_dwell_s",
      "hold_release_after_s", "stop_release_ttc_s"}));
  EXPECT_EQ(value(st, "plugin"), "FollowPath");
  EXPECT_EQ(value(st, "mode"), "NORMAL");
  // The far person is passed on one side: a one-person class "{7:L}" or "{7:R}".
  EXPECT_EQ(value(st, "committed_class").rfind("{7:", 0), 0u) << value(st, "committed_class");
  EXPECT_EQ(value(st, "e_rev"), "0.000");
  EXPECT_EQ(value(st, "rho"), "0.000");
  EXPECT_EQ(value(st, "people_topic"), "/people");
  EXPECT_EQ(value(st, "people_count"), "1");
  EXPECT_EQ(value(st, "people_applied_last_cycle"), "1");
  EXPECT_EQ(value(st, "people_age_s"), "0.30");
  EXPECT_EQ(value(st, "people_timeout_s"), "0.50");
  EXPECT_EQ(value(st, "people_stale"), "false");
  EXPECT_EQ(value(st, "people_tf_failures"), "0");
  EXPECT_EQ(value(st, "people_tf_last_failure_age_s"), "never");
  EXPECT_EQ(value(st, "seconds_since_compute"), "0.20");
  EXPECT_EQ(value(st, "last_command").rfind("drive (v=0.45", 0), 0u) << value(st, "last_command");
  EXPECT_EQ(value(st, "stop_release_dwell_s"), "-1.00");
  EXPECT_EQ(value(st, "hold_release_after_s"), "-1.00");
  EXPECT_EQ(value(st, "stop_release_ttc_s"), "8.00");
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
        if (s.name == "controller_server: compass (FollowPath)") {++received;}
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

// The boundary rule itself is ROS-free and compiled everywhere, so these run on
// Jazzy too (only the Humble adapter path calls it).
TEST(TaskBoundary, NewPlanAfterIdleRule)
{
  compass_nav2::TaskBoundary b;
  b.configure(0.5, 20.0);
  EXPECT_DOUBLE_EQ(b.effectiveThreshold(), 0.5);
  EXPECT_FALSE(b.newPlanStartsTask(50.0));  // no control call has returned yet
  b.controlReturned(10.0);
  EXPECT_FALSE(b.newPlanStartsTask(10.05));  // in-loop update
  EXPECT_FALSE(b.newPlanStartsTask(10.5));   // boundary excluded
  EXPECT_TRUE(b.newPlanStartsTask(10.51));
  b.controlReturned(10.6);                   // the loop ran again
  EXPECT_FALSE(b.newPlanStartsTask(10.65));
  b.configure(0.0, 20.0);                    // disabled
  b.controlReturned(10.0);
  EXPECT_FALSE(b.newPlanStartsTask(99.0));
}

TEST(TaskBoundary, ThresholdNeverBelowThreeControlPeriods)
{
  compass_nav2::TaskBoundary b;
  b.configure(0.05, 20.0);  // one period: raised to 3 x 0.05 s
  EXPECT_DOUBLE_EQ(b.minimumThreshold(), 0.15);
  EXPECT_DOUBLE_EQ(b.effectiveThreshold(), 0.15);
  b.controlReturned(10.0);
  EXPECT_FALSE(b.newPlanStartsTask(10.1));  // two periods late: not a new task
  EXPECT_TRUE(b.newPlanStartsTask(10.16));
  b.configure(0.5, 2.0);    // slow controller: 3 x 0.5 s
  EXPECT_DOUBLE_EQ(b.effectiveThreshold(), 1.5);
}

TEST(TaskBoundary, IdleIsMeasuredFromTheReturnOfTheLastCall)
{
  // A call that started at 10.0 but blocked until 12.0 returns at 12.0; a plan
  // 0.1 s after that is an in-loop update, not a new task.
  compass_nav2::TaskBoundary b;
  b.configure(0.5, 20.0);
  b.controlReturned(12.0);
  EXPECT_FALSE(b.newPlanStartsTask(12.1));
}

TEST(TaskBoundary, ConfigureUsesAtLeastThreePeriods)
{
  Fixture f(Params{{"FollowPath.task_gap_reset_s", 0.05}});  // controller_frequency 20 Hz
  RuntimeProbe controller;
  f.configure(controller);
  EXPECT_DOUBLE_EQ(controller.taskBoundary().effectiveThreshold(), 0.15);
  controller.cleanup();
}

namespace
{
constexpr bool kHumbleTaskBoundary =
#if defined(COMPASS_NAV2_CONTROLLER_HAS_RESET) && !COMPASS_NAV2_CONTROLLER_HAS_RESET
  true;
#else
  false;
#endif

compass_msgs::msg::People nobody()
{
  compass_msgs::msg::People msg;
  msg.header.frame_id = "map";
  return msg;
}

// Latches STOP at t, then the person leaves (empty, fresh people input).
void latchStop(RuntimeProbe & controller)
{
  controller.setPlan(straightPath());
  controller.deliver(oncoming());
  controller.computeVelocityCommands(startPose(), moving(), nullptr);
  ASSERT_EQ(controller.mode(), compass::Mode::STOP);
}

compass::Mode tick(RuntimeProbe & controller)
{
  controller.deliver(nobody());
  controller.computeVelocityCommands(startPose(), geometry_msgs::msg::Twist(), nullptr);
  return controller.mode();
}

nav_msgs::msg::Path otherPath()
{
  auto path = straightPath();
  path.poses.back().pose.position.y = 2.6;  // a different geometry
  return path;
}
}  // namespace

// In-loop plan updates (preemption, replanning) arrive within one control
// iteration of the last call: never a new task.
TEST(TaskBoundary, InLoopPlanUpdateKeepsState)
{
  Fixture f;
  RuntimeProbe controller;
  double t = 100.0;
  controller.useSteady(&t);
  f.configure(controller);
  latchStop(controller);
  for (int i = 0; i < 20; ++i) {
    t += 0.05;
    controller.setPlan(i % 2 ? straightPath() : otherPath());
    EXPECT_EQ(tick(controller), compass::Mode::STOP);
  }
  controller.cleanup();
}

// A stalled control loop without a new plan keeps STOP (no mid-task reset).
TEST(TaskBoundary, GapWithoutNewPlanKeepsState)
{
  Fixture f;
  RuntimeProbe controller;
  double t = 100.0;
  controller.useSteady(&t);
  f.configure(controller);
  latchStop(controller);
  t += 5.0;
  EXPECT_EQ(tick(controller), compass::Mode::STOP);
  controller.cleanup();
}

// A plan arriving after the loop was idle > task_gap_reset_s is a new FollowPath
// action: Humble resets (parity with Jazzy's reset() at action end); on Jazzy
// this path does nothing and only reset() clears STOP.
TEST(TaskBoundary, NewPlanAfterIdleResetsOnlyWithoutResetHook)
{
  Fixture f;
  RuntimeProbe controller;
  double t = 100.0;
  controller.useSteady(&t);
  f.configure(controller);
  latchStop(controller);
  t += 0.6;  // FollowPath aborted; the BT sends a new goal 0.6 s later
  controller.setPlan(straightPath());
  EXPECT_EQ(controller.mode(), kHumbleTaskBoundary ? compass::Mode::NORMAL : compass::Mode::STOP);
  t += 0.05;
  controller.deliver(nobody());
  const auto cmd =
    controller.computeVelocityCommands(startPose(), geometry_msgs::msg::Twist(), nullptr);
  if (kHumbleTaskBoundary) {
    EXPECT_GT(cmd.twist.linear.x, 0.0);
  } else {
    EXPECT_DOUBLE_EQ(cmd.twist.linear.x, 0.0);
  }
  controller.cleanup();
}

// A control call blocked for 0.7 s (costmap mutex held elsewhere) is not idle
// time: a plan right after it returns is an in-loop update (real steady clock).
TEST(TaskBoundary, BlockedComputeIsNotIdle)
{
  Fixture f;
  RuntimeProbe controller;
  f.configure(controller);
  latchStop(controller);
  std::thread holder;
  {
    std::unique_lock<nav2_costmap_2d::Costmap2D::mutex_t> hold(
      *f.costmap->getCostmap()->getMutex());
    holder = std::thread([&] {tick(controller);});  // blocks on the costmap mutex
    std::this_thread::sleep_for(std::chrono::milliseconds(700));
  }
  holder.join();
  controller.setPlan(otherPath());
  EXPECT_EQ(controller.mode(), compass::Mode::STOP);
  controller.cleanup();
}

// A fast retry (new plan after a shorter idle period) keeps the latched state.
TEST(TaskBoundary, FastRetryKeepsState)
{
  Fixture f;
  RuntimeProbe controller;
  double t = 100.0;
  controller.useSteady(&t);
  f.configure(controller);
  latchStop(controller);
  t += 0.3;
  controller.setPlan(straightPath());
  EXPECT_EQ(controller.mode(), compass::Mode::STOP);
  controller.cleanup();
}

// ---- Opt-in STOP/HOLD release (beyond the published method; off by default) ----

namespace
{
compass_msgs::msg::People person(double x, double y, double vx, const char * frame = "map")
{
  compass_msgs::msg::People msg;
  msg.header.frame_id = frame;
  compass_msgs::msg::Person p;
  p.id = 3;
  p.x = x;
  p.y = y;
  p.vx = vx;
  msg.people.push_back(p);
  return msg;
}

compass_msgs::msg::People empty()
{
  compass_msgs::msg::People msg;
  msg.header.frame_id = "map";
  return msg;
}

// A configured controller whose steady and decision (ROS) clocks the test drives.
struct Rig
{
  Fixture f;
  RuntimeProbe c;
  double t{100.0};

  explicit Rig(const Params & p = {})
  : f(p)
  {
    c.useSteady(&t);
    f.configure(c);
    c.useRosTime(t);
    c.setPlan(straightPath());
  }
  ~Rig() {c.cleanup();}

  // One control call dt later; `msg` (if any) arrives just before it.
  geometry_msgs::msg::TwistStamped step(
    double dt, const compass_msgs::msg::People * msg, double speed = 0.0)
  {
    t += dt;
    c.useRosTime(t);
    if (msg) {c.deliver(*msg);}
    geometry_msgs::msg::Twist v;
    v.linear.x = speed;
    return c.computeVelocityCommands(startPose(), v, nullptr);
  }

  // STOP with the committed class {3:L|R}: the robot commits to passing person 3
  // while it is far, then the person walks at the moving robot (TTC 0.48 s).
  void stopCommitted()
  {
    const auto far = person(3.5, 2.5, 0.0);
    step(0.0, &far, 0.45);
    ASSERT_EQ(c.mode(), compass::Mode::NORMAL);
    const auto oncoming = person(1.2, 2.5, -1.0);
    step(1.0, &oncoming, 0.45);
    ASSERT_EQ(c.mode(), compass::Mode::STOP);
  }

  // STOP with the empty committed class (first decision already in danger).
  void stopUncommitted()
  {
    const auto oncoming = person(1.2, 2.5, -1.0);
    step(0.0, &oncoming, 0.45);
    ASSERT_EQ(c.mode(), compass::Mode::STOP);
  }
};

const Params kRelease{{"FollowPath.stop_release_dwell_s", 0.5}};
}  // namespace

TEST(OptionalRelease, DefaultsLatchStopAndHoldAsPublished)
{
  for (const auto & params : {Params{}, Params{{"FollowPath.stop_release_dwell_s", -1.0},
      {"FollowPath.hold_release_after_s", -1.0}}})
  {
    Rig r(params);
    r.stopUncommitted();
    const auto nobody = empty();
    for (int i = 0; i < 60; ++i) {  // 30 s, the person gone, fresh input
      const auto cmd = r.step(0.5, &nobody);
      EXPECT_EQ(r.c.mode(), compass::Mode::STOP);
      EXPECT_DOUBLE_EQ(cmd.twist.linear.x, 0.0);
      EXPECT_DOUBLE_EQ(cmd.twist.angular.z, 0.0);
    }
    r.c.forceHold();
    for (int i = 0; i < 60; ++i) {
      EXPECT_DOUBLE_EQ(r.step(0.5, &nobody).twist.linear.x, 0.0);
      EXPECT_EQ(r.c.mode(), compass::Mode::HOLD);
    }
  }
}

// The halted robot's measured-speed TTC to a person standing 0.7 m ahead is the
// 10 s default; at the resume speed (0.45 m/s) it is 1.56 s < ttc_min.
TEST(OptionalRelease, StandingPersonInsideResumeEnvelopeNeverReleases)
{
  Rig r(kRelease);
  r.stopCommitted();
  const auto standing = person(1.2, 2.5, 0.0);
  for (int i = 0; i < 60; ++i) {  // 30 s
    const auto cmd = r.step(0.5, &standing);
    ASSERT_EQ(r.c.mode(), compass::Mode::STOP) << "released after " << (i + 1) * 0.5 << " s";
    EXPECT_DOUBLE_EQ(cmd.twist.linear.x, 0.0);
  }
}

TEST(OptionalRelease, PersonWalksAwayReleasesAfterDwellWithoutLurch)
{
  Rig r(kRelease);
  r.stopCommitted();
  const auto away = person(4.5, 2.5, 0.0);  // TTC at 0.45 m/s: 8.9 s
  r.step(0.1, &away);
  EXPECT_EQ(r.c.mode(), compass::Mode::STOP);
  r.step(0.3, &away);
  EXPECT_EQ(r.c.mode(), compass::Mode::STOP);  // 0.3 s of the 0.5 s dwell
  auto cmd = r.step(0.25, &away);
  EXPECT_EQ(r.c.mode(), compass::Mode::NORMAL);
  EXPECT_DOUBLE_EQ(cmd.twist.linear.x, 0.0);  // the releasing cycle still stops
  for (int i = 0; i < 10; ++i) {  // driving again: no STOP -> drive -> STOP lurch
    cmd = r.step(0.05, &away, 0.45);
    EXPECT_EQ(r.c.mode(), compass::Mode::NORMAL);
    EXPECT_GT(cmd.twist.linear.x, 0.0);
  }
}

TEST(OptionalRelease, StaleOrMissingPeopleNeverRelease)
{
  Rig r(kRelease);
  r.stopUncommitted();
  const auto nobody = empty();
  r.step(0.1, &nobody);  // the last message: nobody there
  for (int i = 0; i < 300; ++i) {  // 30 s with the tracker silent
    r.step(0.1, nullptr);
    ASSERT_EQ(r.c.mode(), compass::Mode::STOP) << "released after " << (i + 1) * 0.1 << " s";
  }
}

TEST(OptionalRelease, PeopleTfFailureNeverReleases)
{
  Rig r(kRelease);
  r.stopUncommitted();
  const auto unknown_frame = person(4.5, 2.5, 0.0, "tracker_frame");  // no TF to map
  for (int i = 0; i < 60; ++i) {
    r.step(0.5, &unknown_frame);
    ASSERT_EQ(r.c.mode(), compass::Mode::STOP) << "released after " << (i + 1) * 0.5 << " s";
  }
}

TEST(OptionalRelease, DwellRestartsWhenDangerReturns)
{
  Rig r(kRelease);
  r.stopCommitted();
  const auto away = person(4.5, 2.5, 0.0);
  const auto standing = person(1.2, 2.5, 0.0);
  r.step(0.1, &away);
  r.step(0.3, &away);
  r.step(0.05, &standing);  // back inside the envelope: dwell restarts
  EXPECT_EQ(r.c.mode(), compass::Mode::STOP);
  r.step(0.1, &away);
  r.step(0.3, &away);
  EXPECT_EQ(r.c.mode(), compass::Mode::STOP);  // 0.3 s of the new dwell
  r.step(0.25, &away);
  EXPECT_EQ(r.c.mode(), compass::Mode::NORMAL);
}

// No dwell credit across a period in which the controller was not running: a
// check more than max(2 control periods, people_timeout_s) after the previous
// one restarts the dwell.
TEST(OptionalRelease, DwellNotCreditedAcrossIdleGap)
{
  Rig r(kRelease);
  r.stopCommitted();
  const auto away = person(4.5, 2.5, 0.0);
  r.step(0.1, &away);  // clear since here
  r.step(1.0, &away);  // 1.0 s without a control call: restart, not 1.1 s of credit
  EXPECT_EQ(r.c.mode(), compass::Mode::STOP);
  r.step(0.3, &away);
  EXPECT_EQ(r.c.mode(), compass::Mode::STOP);
  r.step(0.25, &away);
  EXPECT_EQ(r.c.mode(), compass::Mode::NORMAL);
}

// HOLD is released into a gated STOP, never straight to NORMAL; with nobody
// around the STOP gate then passes after its dwell.
TEST(OptionalRelease, HoldReleasesIntoGatedStop)
{
  Rig r(Params{{"FollowPath.hold_release_after_s", 4.0},
      {"FollowPath.stop_release_dwell_s", 0.5}});
  r.c.forceHold();
  const auto nobody = empty();
  EXPECT_DOUBLE_EQ(r.step(0.05, &nobody).twist.linear.x, 0.0);  // HOLD since here
  for (int i = 0; i < 7; ++i) {  // up to 3.5 s in HOLD
    EXPECT_DOUBLE_EQ(r.step(0.5, &nobody).twist.linear.x, 0.0);
    EXPECT_EQ(r.c.mode(), compass::Mode::HOLD);
  }
  auto cmd = r.step(0.55, &nobody);  // 4.05 s: HOLD -> STOP, gate dwell starts
  EXPECT_EQ(r.c.mode(), compass::Mode::STOP);
  EXPECT_DOUBLE_EQ(cmd.twist.linear.x, 0.0);
  cmd = r.step(0.25, &nobody);
  EXPECT_EQ(r.c.mode(), compass::Mode::STOP);
  EXPECT_DOUBLE_EQ(cmd.twist.linear.x, 0.0);
  cmd = r.step(0.25, &nobody);  // 0.5 s dwell: released, this command still zero
  EXPECT_EQ(r.c.mode(), compass::Mode::NORMAL);
  EXPECT_DOUBLE_EQ(cmd.twist.linear.x, 0.0);
  EXPECT_GT(r.step(0.05, &nobody).twist.linear.x, 0.0);
}

TEST(OptionalRelease, HoldReleaseRequiresStopRelease)
{
  Fixture f(Params{{"FollowPath.hold_release_after_s", 3.0}});
  RuntimeProbe controller;
  try {
    f.configure(controller);
    ADD_FAILURE() << "configure accepted hold_release_after_s without stop_release_dwell_s";
  } catch (const std::invalid_argument & e) {
    EXPECT_NE(std::string(e.what()).find("requires the STOP release"), std::string::npos)
      << e.what();
  }
}

TEST(OptionalRelease, RejectsUnsafeDurations)
{
  const std::vector<rclcpp::Parameter> bad = {
    {"FollowPath.stop_release_dwell_s", -0.5},
    {"FollowPath.hold_release_after_s", -0.5},
    {"FollowPath.hold_release_after_s", 0.0},   // would disable the thrash guard
    {"FollowPath.hold_release_after_s", 2.9},   // shorter than W = 3 s
    {"FollowPath.stop_release_ttc_s", 1.5},     // below ttc_min = 2 s
  };
  for (const auto & p : bad) {
    Fixture f(Params{p});
    RuntimeProbe controller;
    EXPECT_THROW(f.configure(controller), std::invalid_argument) << p.get_name();
  }
  Fixture f(Params{{"FollowPath.hold_release_after_s", 3.0},
      {"FollowPath.stop_release_dwell_s", 0.5}});
  RuntimeProbe controller;
  EXPECT_NO_THROW(f.configure(controller));
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
