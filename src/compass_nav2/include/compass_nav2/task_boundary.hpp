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

#ifndef COMPASS_NAV2__TASK_BOUNDARY_HPP_
#define COMPASS_NAV2__TASK_BOUNDARY_HPP_

#include <algorithm>
#include <cmath>

namespace compass_nav2
{

// Task boundary for Nav2 releases without Controller::reset() (Humble).
// ROS-free and compiled on every distribution so it is tested everywhere; only
// the Humble adapter path calls it.
//
// A new plan starts a new task only when the control loop has been idle (no
// control call has returned) for longer than the effective threshold. In-loop
// plan updates (goal preemption) arrive within one control iteration of the
// previous return and never qualify. Times are steady-clock seconds.
class TaskBoundary
{
public:
  static constexpr double kMinPeriods = 3.0;

  // threshold_s <= 0 disables the boundary. Otherwise the effective threshold
  // is max(threshold_s, 3 / controller_frequency_hz), so a threshold below three
  // control periods cannot classify an ordinary slow cycle as idle.
  void configure(double threshold_s, double controller_frequency_hz)
  {
    const double period = std::isfinite(controller_frequency_hz) &&
      controller_frequency_hz > 0.0 ? 1.0 / controller_frequency_hz : 0.05;
    min_threshold_s_ = kMinPeriods * period;
    effective_s_ = threshold_s > 0.0 ? std::max(threshold_s, min_threshold_s_) : 0.0;
    has_return_ = false;
  }

  bool enabled() const {return effective_s_ > 0.0;}
  double effectiveThreshold() const {return effective_s_;}
  double minimumThreshold() const {return min_threshold_s_;}

  // Call when a control call returns (not when it starts: a call blocked on a
  // lock is not idle time).
  void controlReturned(double now_s)
  {
    last_return_s_ = now_s;
    has_return_ = true;
  }

  // A plan arrived at now_s: true when it starts a new task.
  bool newPlanStartsTask(double now_s) const
  {
    return enabled() && has_return_ && idleFor(now_s) > effective_s_;
  }

  double idleFor(double now_s) const {return has_return_ ? now_s - last_return_s_ : 0.0;}

private:
  double effective_s_{0.0};
  double min_threshold_s_{0.15};
  double last_return_s_{0.0};
  bool has_return_{false};
};

}  // namespace compass_nav2

#endif  // COMPASS_NAV2__TASK_BOUNDARY_HPP_
