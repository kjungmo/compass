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

#ifndef TEST_COSTMAP_HPP_
#define TEST_COSTMAP_HPP_

#include <chrono>
#include <thread>

#include "nav2_costmap_2d/costmap_2d_ros.hpp"

// A Costmap2DROS whose cleanup does not stall when it follows configure closely,
// as it does in a test.
//
// Cause (Nav2 1.1.20 and 1.3.11): Costmap2DROS::on_configure starts a
// nav2_util::NodeThread, a new thread that calls executor_->spin(), and
// on_cleanup destroys it, which is executor_->cancel() followed by join().
// cancel() clears the executor's `spinning` flag and spin() sets it when it
// starts. If cleanup runs before the new thread has entered spin(), the cancel
// is lost and the join never returns. Measured with a loop of construct,
// configure, cleanup (300 cycles per run): 6 of 40 runs stalled on Humble and
// 26 of 30 on Jazzy; every Jazzy stall and 4 of the 6 on Humble had the
// executor not spinning when cleanup began and spinning afterwards. Waiting
// for is_spinning() first: 0 of 40 runs on Jazzy, 1 of 40 on Humble.
//
// That remaining Humble stall began with the executor already spinning, so it
// has another cause, which was not identified. The 10 ms pause below is a
// mitigation for it, not a fix of a known cause: with the wait and the pause,
// 0 of 80 runs (24 000 cycles) stalled on Humble and 0 of 40 on Jazzy.
// test_controller_runtime itself, 100 runs each: 2 stalls on Humble and 7 on
// Jazzy with plain cleanup(), none on either with this class.
class TestCostmap : public nav2_costmap_2d::Costmap2DROS
{
public:
  using nav2_costmap_2d::Costmap2DROS::Costmap2DROS;

  void cleanupOnceSpinning()
  {
    // Bounded: never wait for a thread that was not started (not configured).
    const auto give_up = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (executor_ && !executor_->is_spinning() &&
      std::chrono::steady_clock::now() < give_up)
    {
      std::this_thread::yield();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cleanup();
  }
};

#endif  // TEST_COSTMAP_HPP_
