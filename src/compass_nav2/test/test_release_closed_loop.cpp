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

// Kinematic closed-loop checks of the optional STOP release (beyond the
// published method, off by default). The real plugin runs at 20 Hz on a
// configured (empty) costmap with the default parameter values; its commanded
// twist is fed back as the measured velocity and integrated as a unicycle, and
// one tracked person moves at constant velocity. The robot starts halted in
// STOP. This is a kinematic model only: no dynamics, sensing or Gazebo.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "compass_nav2/compass_controller.hpp"

namespace
{
using Params = std::vector<rclcpp::Parameter>;

constexpr double kDt = 0.05;           // controller_frequency 20 Hz
constexpr double kDefaultReleaseTtc = 8.0;  // stop_release_ttc_s default
constexpr double kTtcStop = 0.8;       // ttc_stop default
constexpr double kTtcMin = 2.0;        // ttc_min default
// Driving checked after a release in the walk-away case.
constexpr double kNoRestop = kDefaultReleaseTtc - kTtcStop;  // 7.2 s
// Guaranteed bound for a hazard whose closing speed does not increase: the
// committed class leaves the safe set once TTC < ttc_min, and the thrash guard
// can turn the following safety-branch cycles into HOLD within a few periods,
// so the bound is stop_release_ttc_s - ttc_min, not - ttc_stop.
constexpr double kNoRestopGuaranteed = kDefaultReleaseTtc - kTtcMin;  // 6.0 s

class LoopProbe : public compass_nav2::CompassController
{
public:
  void useClocks(const double * t)
  {
    steady_now_ = [t] {return *t;};
    ros_ = std::make_shared<rclcpp::Clock>(RCL_ROS_TIME);
    if (rcl_enable_ros_time_override(ros_->get_clock_handle()) != RCL_RET_OK) {
      throw std::runtime_error("cannot enable the ROS time override");
    }
    clock_ = ros_;
    t_ = t;
    sync();
  }
  void sync()
  {
    if (rcl_set_ros_time_override(ros_->get_clock_handle(), static_cast<int64_t>(*t_ * 1e9)) !=
      RCL_RET_OK)
    {
      throw std::runtime_error("cannot set the ROS time override");
    }
  }
  void deliver(const compass_msgs::msg::People & msg)
  {
    peopleCallback(std::make_shared<compass_msgs::msg::People>(msg));
  }
  void forceStop() {state_.mode = compass::Mode::STOP;}
  void forceHold() {state_.mode = compass::Mode::HOLD;}
  compass::Mode mode() const {return state_.mode;}

private:
  rclcpp::Clock::SharedPtr ros_;
  const double * t_{nullptr};
};

struct Walker
{
  double x, y, vx, vy;
};

struct Outcome
{
  double release_time = -1.0;          // s after start; -1: never released
  double first_restop_after_release = std::numeric_limits<double>::infinity();
  double min_distance = std::numeric_limits<double>::infinity();
  double travelled = 0.0;              // m along x
  bool ever_normal = false;
  double hold_exit_time = -1.0;        // first time out of HOLD (start in HOLD)
  double first_motion_time = -1.0;     // first nonzero linear command
  double max_speed = 0.0;              // largest commanded linear speed
};

// Robot at (0.5, 2.5) facing +x on a straight 4 m path, halted in STOP (or HOLD).
Outcome simulate(
  const char * label, const Params & params, Walker p, double duration_s,
  compass::Mode start = compass::Mode::STOP)
{
  rclcpp::NodeOptions options;
  options.parameter_overrides(params);
  auto node = std::make_shared<rclcpp_lifecycle::LifecycleNode>("controller_server", options);
  auto costmap = std::make_shared<nav2_costmap_2d::Costmap2DROS>("closed_loop_costmap");
  costmap->configure();
  auto tf = std::make_shared<tf2_ros::Buffer>(node->get_clock());
  LoopProbe c;
  double t = 100.0;
  c.configure(node, "FollowPath", tf, costmap);
  c.useClocks(&t);

  nav_msgs::msg::Path path;
  path.header.frame_id = "map";
  for (int i = 0; i <= 40; ++i) {
    geometry_msgs::msg::PoseStamped ps;
    ps.header.frame_id = "map";
    ps.pose.position.x = 0.5 + 0.1 * i;
    ps.pose.position.y = 2.5;
    ps.pose.orientation.w = 1.0;
    path.poses.push_back(ps);
  }
  c.setPlan(path);
  if (start == compass::Mode::HOLD) {c.forceHold();} else {c.forceStop();}

  double x = 0.5, y = 2.5, th = 0.0, v = 0.0, w = 0.0;
  Outcome r;
  const int steps = static_cast<int>(std::lround(duration_s / kDt));
  for (int k = 1; k <= steps; ++k) {
    t += kDt;
    c.sync();
    p.x += p.vx * kDt;
    p.y += p.vy * kDt;
    compass_msgs::msg::People msg;
    msg.header.frame_id = "map";
    compass_msgs::msg::Person person;
    person.id = 3;
    person.x = p.x;
    person.y = p.y;
    person.vx = p.vx;
    person.vy = p.vy;
    msg.people.push_back(person);
    c.deliver(msg);

    geometry_msgs::msg::PoseStamped pose;
    pose.header.frame_id = "map";
    pose.pose.position.x = x;
    pose.pose.position.y = y;
    pose.pose.orientation.z = std::sin(th / 2);
    pose.pose.orientation.w = std::cos(th / 2);
    geometry_msgs::msg::Twist measured;
    measured.linear.x = v;
    measured.angular.z = w;
    const auto cmd = c.computeVelocityCommands(pose, measured, nullptr);
    v = cmd.twist.linear.x;
    w = cmd.twist.angular.z;
    x += v * std::cos(th) * kDt;
    y += v * std::sin(th) * kDt;
    th += w * kDt;

    const double elapsed = k * kDt;
    const compass::Mode m = c.mode();
    if (start == compass::Mode::HOLD && r.hold_exit_time < 0.0 && m != compass::Mode::HOLD) {
      r.hold_exit_time = elapsed;
    }
    if (v > 0.0 && r.first_motion_time < 0.0) {r.first_motion_time = elapsed;}
    r.max_speed = std::max(r.max_speed, v);
    if (m == compass::Mode::NORMAL) {
      r.ever_normal = true;
      if (r.release_time < 0.0) {r.release_time = elapsed;}
    } else if (r.release_time >= 0.0 && std::isinf(r.first_restop_after_release)) {
      r.first_restop_after_release = elapsed - r.release_time;
    }
    r.min_distance = std::min(r.min_distance, std::hypot(p.x - x, p.y - y));
  }
  r.travelled = x - 0.5;
  std::printf(
    "[ closed-loop ] %-34s release %6.2f s  first STOP/HOLD after release %6.2f s  "
    "min distance %5.2f m  travelled %5.2f m  left HOLD %6.2f s  first motion %6.2f s\n",
    label, r.release_time, r.first_restop_after_release, r.min_distance, r.travelled,
    r.hold_exit_time, r.first_motion_time);
  c.cleanup();
  costmap->cleanup();
  return r;
}

const Params kRelease{{"FollowPath.stop_release_dwell_s", 0.5}};
const Params kLatch{};  // published behaviour
const Params kBoth{{"FollowPath.stop_release_dwell_s", 0.5},
  {"FollowPath.hold_release_after_s", 3.0}};
}  // namespace

// (a) A person standing 1.7 m ahead on the path: TTC at the resume speed
// (0.45 m/s) is 3.8 s, inside the release margin, so STOP stays latched.
TEST(ReleaseClosedLoop, StandingPersonAheadNeverReleases)
{
  const Outcome r = simulate("(a) standing 1.7 m ahead", kRelease, {2.2, 2.5, 0.0, 0.0}, 30.0);
  EXPECT_FALSE(r.ever_normal) << "released at " << r.release_time << " s";
  EXPECT_DOUBLE_EQ(r.travelled, 0.0);
}

// (b) A person crossing the path ahead, from (2.5, 4.5) at 0.6 m/s: with the
// release enabled the robot never gets closer to the person than when STOP stays
// latched as published.
TEST(ReleaseClosedLoop, CrossingPersonNoCloserThanWithoutRelease)
{
  const Walker crossing{2.5, 4.5, 0.0, -0.6};
  const Outcome latched = simulate("(b) crossing, release disabled", kLatch, crossing, 20.0);
  const Outcome released = simulate("(b) crossing, release enabled", kRelease, crossing, 20.0);
  EXPECT_FALSE(latched.ever_normal);
  ASSERT_TRUE(released.ever_normal) << "release never happened";
  EXPECT_GT(released.travelled, 0.0);
  EXPECT_GE(released.min_distance, latched.min_distance - 1e-9)
    << "release at " << released.release_time << " s, min distance "
    << released.min_distance << " m vs " << latched.min_distance << " m";
}

// (c) A person behind the halted robot closing at 0.6 m/s. At the resume speed
// the closing speed is only 0.15 m/s (TTC 6.7 s from 1 m), but at the measured
// speed (0) the TTC is 1.67 s from 1 m and 2.7 s from 1.6 m: no release.
// From 1.0 m the core itself already escalates (the committed class is unsafe
// at the measured speed); from 1.6 m the committed class is still safe at the
// measured speed during the dwell, which is where a resume-speed-only check
// released.
TEST(ReleaseClosedLoop, PersonClosingFromBehindNoRelease)
{
  for (const double behind : {1.0, 1.6}) {
    const Outcome r = simulate(behind < 1.3 ? "(c) closing from 1.0 m behind" : "(c) closing from 1.6 m behind", kRelease, {0.5 - behind, 2.5, 0.6, 0.0}, 1.2);
    EXPECT_FALSE(r.ever_normal) << behind << " m behind: released at " << r.release_time << " s";
  }
}

// (d) The person walks away ahead: release after the dwell, then no STOP/HOLD for
// at least stop_release_ttc_s - ttc_stop of simulated driving.
TEST(ReleaseClosedLoop, PersonWalksAwayReleasesWithoutRestop)
{
  const Outcome r = simulate("(d) walks away", kRelease, {1.2, 2.5, 1.0, 0.0}, 0.5 + 0.5 + kNoRestop + 0.5);
  ASSERT_TRUE(r.ever_normal);
  EXPECT_GE(r.release_time, 0.5);
  EXPECT_LE(r.release_time, 1.0);
  EXPECT_GE(r.first_restop_after_release, kNoRestop);
  EXPECT_GT(r.travelled, 0.45 * kNoRestop * 0.9);
}

// (e) Normal approach behaviour: a person standing beyond resume speed x
// stop_release_ttc_s (0.45 x 8 = 3.6 m) does not block the release; the robot
// then drives towards them and the published safety ladder stops it again, but
// no sooner than stop_release_ttc_s - ttc_min after the release.
TEST(ReleaseClosedLoop, FarStandingPersonReleaseThenNoEarlyRestop)
{
  const Outcome r = simulate("(e) standing 3.9 m ahead", kRelease, {4.4, 2.5, 0.0, 0.0}, 30.0);
  ASSERT_TRUE(r.ever_normal);
  EXPECT_GE(r.first_restop_after_release, kNoRestopGuaranteed)
    << "released at " << r.release_time << " s";
  EXPECT_FALSE(std::isinf(r.first_restop_after_release));  // it does stop in front of them
}

// ---- HOLD release (needs the STOP release; HOLD -> gated STOP, never NORMAL) ----

// (f) A person standing ahead while the robot rests in HOLD (as the published
// core leaves it after sustained danger): with both options on, every HOLD
// release becomes a gated STOP that does not pass, so the robot never moves.
TEST(ReleaseClosedLoop, HoldReleaseNeverCreepsTowardsStandingPerson)
{
  for (const double ahead : {1.0, 1.7, 3.0}) {
    const std::string label = "(f) HOLD, standing " + std::to_string(ahead).substr(0, 3) + " m";
    const Outcome r = simulate(
      label.c_str(), kBoth, {0.5 + ahead, 2.5, 0.0, 0.0}, 30.0, compass::Mode::HOLD);
    EXPECT_DOUBLE_EQ(r.max_speed, 0.0) << ahead << " m ahead";
    EXPECT_DOUBLE_EQ(r.travelled, 0.0) << ahead << " m ahead";
    EXPECT_FALSE(r.ever_normal) << ahead << " m ahead: released at " << r.release_time << " s";
  }
}

// (g) The person walks away while the robot is in HOLD: HOLD -> STOP after
// hold_release_after_s, STOP -> NORMAL after the gate's dwell, then it drives;
// no motion before the gate passes.
TEST(ReleaseClosedLoop, HoldThenGatedStopThenDriveWhenPersonLeaves)
{
  const Outcome r = simulate(
    "(g) HOLD, person walks away", kBoth, {1.2, 2.5, 1.0, 0.0}, 10.0, compass::Mode::HOLD);
  // The HOLD timer starts at the first control call (0.05 s), so HOLD ends at the
  // first call >= 3 s later: 3.05 s or, with floating-point rounding, 3.10 s.
  EXPECT_GE(r.hold_exit_time, 3.05 - 1e-9);
  EXPECT_LE(r.hold_exit_time, 3.10 + 1e-9);
  ASSERT_TRUE(r.ever_normal);
  EXPECT_GE(r.release_time, r.hold_exit_time + 0.5);   // the STOP gate's dwell
  EXPECT_GT(r.first_motion_time, r.release_time);      // nothing moves before the gate
  EXPECT_GT(r.travelled, 0.0);
}

// (h) HOLD release without the STOP release is rejected at configure.
TEST(ReleaseClosedLoop, HoldReleaseWithoutStopReleaseFailsConfigure)
{
  EXPECT_THROW(
    simulate("(h) hold only", Params{{"FollowPath.hold_release_after_s", 3.0}},
    {4.0, 2.5, 0.0, 0.0}, 0.1, compass::Mode::HOLD), std::invalid_argument);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  rclcpp::init(argc, argv);
  const int rc = RUN_ALL_TESTS();
  rclcpp::shutdown();
  return rc;
}
