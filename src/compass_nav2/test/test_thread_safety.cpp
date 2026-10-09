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

// Locking contract: the control call evaluates the costmap only while holding the
// costmap's mutex (the costmap update thread rewrites it concurrently), and
// setSpeedLimit writes the limit under the controller mutex.

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include <gtest/gtest.h>

#include "compass_nav2/compass_controller.hpp"

using namespace std::chrono_literals;

namespace
{
class LockProbe : public compass_nav2::CompassController
{
public:
  explicit LockProbe(std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap = nullptr)
  {
    costmap_ros_ = costmap;
    global_frame_ = "map";
    clock_ = std::make_shared<rclcpp::Clock>(RCL_SYSTEM_TIME);
    core_ = std::make_unique<compass::DecisionCore>(knobs_);
  }
  std::mutex & controllerMutex() {return mutex_;}
  double speedLimit() const {return speed_limit_;}
};

// Runs `call` on another thread while `m` is held; returns true when the call
// waited for the mutex (did not finish while it was held) and finished after.
template<typename Mutex, typename Call>
bool blocksOn(Mutex & m, Call call)
{
  std::atomic<bool> done{false};
  std::thread t;
  {
    std::unique_lock<Mutex> hold(m);
    t = std::thread([&] {call(); done = true;});
    std::this_thread::sleep_for(300ms);
    if (done) {t.join(); return false;}
  }
  t.join();
  return done;
}
}  // namespace

TEST(ThreadSafety, ControlCallHoldsTheCostmapMutex)
{
  auto costmap = std::make_shared<nav2_costmap_2d::Costmap2DROS>("lock_probe_costmap");
  costmap->configure();
  ASSERT_NE(costmap->getCostmap(), nullptr);
  LockProbe controller(costmap);
  geometry_msgs::msg::PoseStamped pose;
  pose.header.frame_id = "map";
  pose.pose.orientation.w = 1;
  EXPECT_TRUE(
    blocksOn(
      *costmap->getCostmap()->getMutex(),
      [&] {controller.computeVelocityCommands(pose, geometry_msgs::msg::Twist(), nullptr);}));
  costmap->cleanup();
}

TEST(ThreadSafety, SetSpeedLimitTakesTheControllerMutex)
{
  LockProbe controller;
  EXPECT_TRUE(blocksOn(controller.controllerMutex(), [&] {controller.setSpeedLimit(0.2, false);}));
  EXPECT_DOUBLE_EQ(controller.speedLimit(), 0.2);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  rclcpp::init(argc, argv);
  const int rc = RUN_ALL_TESTS();
  rclcpp::shutdown();
  return rc;
}
