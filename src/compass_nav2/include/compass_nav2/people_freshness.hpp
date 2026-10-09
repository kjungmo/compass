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

#ifndef COMPASS_NAV2__PEOPLE_FRESHNESS_HPP_
#define COMPASS_NAV2__PEOPLE_FRESHNESS_HPP_

#include <algorithm>

namespace compass_nav2
{

struct PeopleFreshness
{
  bool received = false;  // any message since configure
  double age_s = 0.0;     // meaningful only when received
  bool stale = true;      // never received, or older than the timeout
};

// Age of the latest people message: the larger of the time since it was received
// (steady clock, so it grows even when /clock stops) and, when the header stamp
// is set, the ROS-clock age of that stamp (so a tracker replaying old tracks or
// stamping with another clock is not mistaken for fresh). A stamp ahead of the
// ROS clock contributes nothing.
inline PeopleFreshness assessPeople(
  bool received, double received_steady_s, double now_steady_s,
  double stamp_s, double ros_now_s, double timeout_s)
{
  PeopleFreshness f;
  f.received = received;
  if (!received) {return f;}
  f.age_s = std::max(0.0, now_steady_s - received_steady_s);
  if (stamp_s > 0.0) {f.age_s = std::max(f.age_s, ros_now_s - stamp_s);}
  f.stale = f.age_s > timeout_s;
  return f;
}

}  // namespace compass_nav2

#endif  // COMPASS_NAV2__PEOPLE_FRESHNESS_HPP_
