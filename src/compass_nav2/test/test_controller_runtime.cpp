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
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "compass_nav2/compass_controller.hpp"
#include "nav2_costmap_2d/cost_values.hpp"
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
  uint64_t tfFailures() const {return tf_failures_.load();}
  const compass_nav2::TaskBoundary & taskBoundary() const {return task_boundary_;}
  double lReal() const {return state_.L_real;}
  std::size_t planSegment() const {return plan_segment_;}
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
      "seconds_since_compute", "last_command"}));
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

// ---------------------------------------------------------------------------
// Local goal on a plan that is not re-issued (static plan). The fixture costmap
// is 5 x 5 m at 0.1 m, free; the path runs along y = 2.5 from x = 0.5 to 4.5.
namespace
{
constexpr double kCruise = 0.45;    // cruise_speed default
constexpr double kBrakeStep = 0.05;  // a_brake 0.5 m/s^2 x the 0.1 s first-call interval

void clearMap(Fixture & f)
{
  nav2_costmap_2d::Costmap2D * map = f.costmap->getCostmap();
  for (unsigned int mx = 0; mx < map->getSizeInCellsX(); ++mx) {
    for (unsigned int my = 0; my < map->getSizeInCellsY(); ++my) {
      map->setCost(mx, my, nav2_costmap_2d::FREE_SPACE);
    }
  }
}

// One costmap for all LocalGoal tests: every test starts with it free and with
// the default lookahead_dist and people_stale_action, and each controller is
// configured on the same node (as after a lifecycle cleanup).
class LocalGoal : public ::testing::Test
{
protected:
  static void SetUpTestSuite() {fixture_ = new Fixture;}
  static void TearDownTestSuite()
  {
    delete fixture_;
    fixture_ = nullptr;
  }
  void SetUp() override
  {
    clearMap(*fixture_);
    if (fixture_->node->has_parameter("FollowPath.lookahead_dist")) {
      fixture_->node->set_parameter(rclcpp::Parameter("FollowPath.lookahead_dist", 1.0));
      fixture_->node->set_parameter(rclcpp::Parameter("FollowPath.people_stale_action", "warn"));
    }
  }
  static Fixture * fixture_;
};
Fixture * LocalGoal::fixture_ = nullptr;

// Lethal cells covering x in [x0, x0 + 0.2], y in [2.3, 2.7] (across the path).
void putBox(Fixture & f, double x0)
{
  nav2_costmap_2d::Costmap2D * map = f.costmap->getCostmap();
  for (double wx = x0 + 0.05; wx < x0 + 0.2; wx += 0.1) {
    for (double wy = 2.35; wy < 2.7; wy += 0.1) {
      unsigned int mx = 0, my = 0;
      ASSERT_TRUE(map->worldToMap(wx, wy, mx, my));
      map->setCost(mx, my, nav2_costmap_2d::LETHAL_OBSTACLE);
    }
  }
}

geometry_msgs::msg::PoseStamped poseAt(double x)
{
  auto pose = startPose();
  pose.pose.position.x = x;
  return pose;
}

geometry_msgs::msg::Twist speed(double v)
{
  geometry_msgs::msg::Twist twist;
  twist.linear.x = v;
  return twist;
}

// The same line as straightPath(), starting at the robot (as a planner issues it).
nav_msgs::msg::Path pathFrom(double x)
{
  nav_msgs::msg::Path path = straightPath();
  path.poses.clear();
  for (double px = x; px < 4.5; px += 0.1) {
    geometry_msgs::msg::PoseStamped p;
    p.header.frame_id = "map";
    p.pose.position.x = px;
    p.pose.position.y = 2.5;
    p.pose.orientation.w = 1.0;
    path.poses.push_back(p);
  }
  path.poses.push_back(straightPath().poses.back());
  return path;
}

// First command of a freshly configured controller at x, moving at cruise speed,
// with the map free except for the box.
double firstCommand(
  Fixture & f, const nav_msgs::msg::Path & plan, double x, double box_x0, bool box = true)
{
  RuntimeProbe controller;
  f.configure(controller);
  clearMap(f);
  if (box) {putBox(f, box_x0);}
  controller.setPlan(plan);
  controller.deliver(nobody());
  const auto cmd = controller.computeVelocityCommands(poseAt(x), speed(kCruise), nullptr);
  EXPECT_EQ(controller.mode(), compass::Mode::NORMAL);
  EXPECT_DOUBLE_EQ(cmd.twist.angular.z, 0.0);
  controller.cleanup();
  return cmd.twist.linear.x;
}

struct Drive
{
  std::vector<double> v;       // command per cycle
  double first_brake_x = -1.0;  // robot x at the first command below cruise
  double final_x = 0.0;
};

// 8 s of a kinematic drive at 20 Hz from x = 0.5 with a box at x = 3.0..3.2. The
// measured speed is the previous command. replan_every = 0: the plan is set once.
Drive drive(Fixture & f, int replan_every)
{
  RuntimeProbe controller;
  double t = 100.0;
  controller.useSteady(&t);
  f.configure(controller);
  controller.useRosTime(t);
  putBox(f, 3.0);
  controller.setPlan(straightPath());
  controller.deliver(nobody());
  Drive d;
  double x = 0.5, v = 0.0;
  for (int i = 0; i < 160; ++i) {
    if (replan_every > 0 && i > 0 && i % replan_every == 0) {
      controller.setPlan(pathFrom(x));
    }
    const auto cmd = controller.computeVelocityCommands(poseAt(x), speed(v), nullptr);
    v = cmd.twist.linear.x;
    d.v.push_back(v);
    if (d.first_brake_x < 0.0 && i > 0 && v < d.v[i - 1]) {d.first_brake_x = x;}
    x += v * 0.05;
    t += 0.05;
    controller.useRosTime(t);
  }
  d.final_x = x;
  controller.cleanup();
  return d;
}
}  // namespace

// Robot positions are 3 cm off the 0.1 m grid so that every 0.1 m ray step falls
// inside a cell, not on a cell edge.
//
// The robot is 1.53 m along a plan that was set once (more than lookahead_dist
// 1.0 m from its first pose). A box whose first cell is hit 0.4 m ahead is closer
// than d_safe 0.5 m: the command is one braking step below cruise. A box hit at
// exactly 0.5 m is not closer than d_safe, and without a box it is cruise.
TEST_F(LocalGoal, StaticPlanBrakesForAnObstacleAhead)
{
  EXPECT_DOUBLE_EQ(firstCommand(*fixture_, straightPath(), 2.03, 2.4, false), kCruise);
  EXPECT_NEAR(firstCommand(*fixture_, straightPath(), 2.03, 2.4), kCruise - kBrakeStep, 1e-9);
  EXPECT_DOUBLE_EQ(firstCommand(*fixture_, straightPath(), 2.03, 2.5), kCruise);
}

// The same robot does not brake for a box 0.4 m BEHIND it (towards the plan start).
TEST_F(LocalGoal, StaticPlanIgnoresAnObstacleBehind)
{
  EXPECT_DOUBLE_EQ(firstCommand(*fixture_, straightPath(), 2.03, 1.5), kCruise);
}

// A plan set once and the same line issued from the robot pose give the same
// first command at every distance from the plan start (0.93 m and 1.03 m are the
// two sides of lookahead_dist).
TEST_F(LocalGoal, StaticPlanMatchesAPlanIssuedFromTheRobot)
{
  for (const double x : {0.53, 1.03, 1.43, 1.53, 1.63, 2.03, 2.53}) {
    const double box = x - 0.03 + 0.4;
    EXPECT_DOUBLE_EQ(
      firstCommand(*fixture_, straightPath(), x, box),
      firstCommand(*fixture_, pathFrom(x), x, box)) << x;
    EXPECT_NEAR(firstCommand(*fixture_, straightPath(), x, box), kCruise - kBrakeStep, 1e-9) << x;
  }
}

// Closed loop with a box at x = 3.0: the command sequence of a plan set once is
// the sequence of a plan re-issued from the robot pose at 1 Hz, cycle for cycle.
// Braking starts when the corridor probe 1 m ahead of the robot (towards the
// local goal) enters the box, i.e. at x >= 2.0; three braking steps later the
// fourth safety intervention latches HOLD, about 0.9 m before the box.
TEST_F(LocalGoal, StaticPlanDrivesLikeA1HzReplan)
{
  const Drive replanned = drive(*fixture_, 20);
  const Drive fixed = drive(*fixture_, 0);
  for (size_t i = 0; i < replanned.v.size(); ++i) {
    std::printf("REPLAN_1HZ_SEQ %zu %.6f\n", i, replanned.v[i]);
  }
  EXPECT_GE(replanned.first_brake_x, 2.0);
  EXPECT_LT(replanned.first_brake_x, 2.0 + kCruise * 0.05);  // within one cycle of travel
  EXPECT_DOUBLE_EQ(replanned.v.back(), 0.0);
  EXPECT_GT(replanned.final_x, replanned.first_brake_x);
  EXPECT_LT(replanned.final_x, 2.1);
  ASSERT_EQ(fixed.v.size(), replanned.v.size());
  for (size_t i = 0; i < fixed.v.size(); ++i) {
    ASSERT_DOUBLE_EQ(fixed.v[i], replanned.v[i]) << "cycle " << i;
  }
  EXPECT_DOUBLE_EQ(fixed.first_brake_x, replanned.first_brake_x);
  EXPECT_DOUBLE_EQ(fixed.final_x, replanned.final_x);
}

// lookahead_dist is the parameter that is used, and "ahead" follows the plan
// round a corner. The plan runs along +x to (2.5, 2.5) and then along +y. A box
// straight on, past the corner, is 0.4 m from the robot in the +x direction.
// lookahead_dist 0.2: the local goal is still on the +x leg, the ray hits the
// box, one braking step. Default 1.0: the local goal is up the +y leg, the ray
// (about 62 degrees to the left) passes the box, cruise.
TEST_F(LocalGoal, LookaheadDistParameterSelectsTheGoalAlongThePlan)
{
  nav_msgs::msg::Path corner = straightPath();
  corner.poses.resize(21);  // x = 0.5 .. 2.5 at y = 2.5
  for (int i = 1; i <= 20; ++i) {
    auto p = corner.poses.back();
    p.pose.position.y = 2.5 + 0.1 * i;
    corner.poses.push_back(p);
  }
  Fixture & f = *fixture_;
  putBox(f, 2.4);
  double v[2];
  for (int near = 0; near < 2; ++near) {
    RuntimeProbe controller;
    if (near) {
      // Declared by the first configure; read again by the next one.
      f.node->set_parameter(rclcpp::Parameter("FollowPath.lookahead_dist", 0.2));
    }
    f.configure(controller);
    controller.setPlan(corner);
    controller.deliver(nobody());
    v[near] = controller.computeVelocityCommands(poseAt(2.03), speed(kCruise), nullptr)
      .twist.linear.x;
    controller.cleanup();
  }
  EXPECT_DOUBLE_EQ(v[0], kCruise);
  EXPECT_NEAR(v[1], kCruise - kBrakeStep, 1e-9);
}

// What the controller remembers about the robot's place on the plan (the event
// table in the README, one block per row):
//   control cycle past the frame and people checks -> the segment the robot is on
//   same geometry republished                      -> kept
//   new geometry                                   -> back to the plan start
//   reset(); on Humble a plan after an idle loop   -> back to the plan start
//   cleanup + configure                            -> back to the plan start
//   cycle stopped by the frame check or by "hold"  -> kept
TEST_F(LocalGoal, RememberedPlanSegmentPerEvent)
{
  Fixture & f = *fixture_;
  RuntimeProbe controller;
  double t = 100.0;
  controller.useSteady(&t);
  f.configure(controller);
  const auto cycle = [&](double x) {
      t += 0.05;
      controller.deliver(nobody());
      controller.computeVelocityCommands(poseAt(x), speed(kCruise), nullptr);
      return controller.planSegment();
    };
  controller.setPlan(straightPath());
  EXPECT_EQ(controller.planSegment(), 0u);
  EXPECT_EQ(cycle(0.53), 0u);    // 0.5 -> 0.6
  EXPECT_EQ(cycle(2.03), 15u);   // 2.0 -> 2.1
  EXPECT_EQ(cycle(1.03), 15u);   // moving back does not rewind it

  auto republished = straightPath();
  republished.header.stamp.sec = 7;
  t += 0.05;
  controller.setPlan(republished);
  EXPECT_EQ(controller.planSegment(), 15u);

  auto wrong_frame = poseAt(3.03);
  wrong_frame.header.frame_id = "odom";
  controller.computeVelocityCommands(wrong_frame, speed(kCruise), nullptr);
  EXPECT_EQ(controller.planSegment(), 15u);

  t += 0.05;
  controller.setPlan(otherPath());
  EXPECT_EQ(controller.planSegment(), 0u);
  EXPECT_EQ(cycle(3.03), 25u);   // 3.0 -> 3.1, found in one cycle

  controller.reset();
  EXPECT_EQ(controller.planSegment(), 0u);
  EXPECT_EQ(cycle(1.03), 5u);    // the same plan again, from its start

  // Humble has no reset(): the same plan arriving after the loop was idle longer
  // than task_gap_reset_s is a new task. On Jazzy it is a republished plan.
  EXPECT_EQ(cycle(2.03), 15u);
  t += 0.6;
  controller.setPlan(otherPath());
  EXPECT_EQ(controller.planSegment(), kHumbleTaskBoundary ? 0u : 15u);
  EXPECT_EQ(cycle(2.03), 15u);

  controller.cleanup();
  f.node->set_parameter(rclcpp::Parameter("FollowPath.people_stale_action", "hold"));
  f.configure(controller);
  EXPECT_EQ(controller.planSegment(), 0u);

  // people_stale_action: hold returns a zero twist before the local goal is taken.
  controller.setPlan(straightPath());
  EXPECT_EQ(cycle(2.03), 15u);
  t += 10.0;  // the people input is now stale
  const auto held = controller.computeVelocityCommands(poseAt(3.03), speed(kCruise), nullptr);
  EXPECT_DOUBLE_EQ(held.twist.linear.x, 0.0);
  EXPECT_EQ(controller.planSegment(), 15u);
  EXPECT_EQ(cycle(3.03), 25u);   // fresh input: the robot is found again
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
