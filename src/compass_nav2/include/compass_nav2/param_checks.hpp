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

#ifndef COMPASS_NAV2__PARAM_CHECKS_HPP_
#define COMPASS_NAV2__PARAM_CHECKS_HPP_

#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

#include "compass_core/accumulator.hpp"
#include "compass_core/knobs.hpp"

namespace compass_nav2
{

// Configure-time range check for one parameter. Throws std::invalid_argument
// naming the parameter, its value and the allowed range when the value is not
// finite or lies outside [lo, hi] (an open end excludes that bound).
inline void requireRange(
  const std::string & name, double value, double lo, double hi,
  bool lo_open = false, bool hi_open = false)
{
  const bool ok = std::isfinite(value) &&
    (lo_open ? value > lo : value >= lo) && (hi_open ? value < hi : value <= hi);
  if (ok) {return;}
  std::ostringstream msg;
  msg << name << " = " << value << " is outside the allowed range "
      << (lo_open ? "(" : "[") << lo << ", ";
  if (std::isinf(hi)) {
    msg << "inf)";
  } else {
    msg << hi << (hi_open ? ")" : "]");
  }
  throw std::invalid_argument(msg.str());
}

constexpr double kInf = std::numeric_limits<double>::infinity();

// Nominal decision period used before the first measured interval (and after a
// clock regression): one controller period. Falls back to Nav2's default 20 Hz.
inline double nominalDecisionDt(double controller_frequency_hz)
{
  return std::isfinite(controller_frequency_hz) && controller_frequency_hz > 0.0 ?
         1.0 / controller_frequency_hz : 0.05;
}

// Decision period for one control call: the measured interval since the last
// call when it is positive, otherwise the nominal period.
inline double decisionDt(bool has_last, double last_now, double now, double nominal_dt)
{
  return (has_last && now > last_now) ? (now - last_now) : nominal_dt;
}

// Empty when discretionary switching is reachable at period dt; otherwise a
// warning with the numbers. The bound is the core's own non-vacuity condition
// (compass::is_non_vacuous) with D_max = w_g + w_s + w_e + w_r, the largest cost
// advantage the normalized cost terms can produce. Lambda is applied per update,
// so the condition depends on dt; it is reported, not changed.
inline std::string switchingVacuityWarning(const compass::Knobs & k, double dt)
{
  const double d_max = k.w_g + k.w_s + k.w_e + k.w_r;
  std::ostringstream msg;
  msg.setf(std::ios::fixed);
  msg.precision(3);
  if (!(d_max > k.delta_floor)) {
    msg << "discretionary class switching is impossible: D_max = w_g+w_s+w_e+w_r = " << d_max
        << " does not exceed delta_floor = " << k.delta_floor;
    return msg.str();
  }
  if (!(dt > 0.0) || compass::is_non_vacuous(d_max, dt, k)) {return "";}
  msg << "discretionary class switching is impossible at dt = " << dt
      << " s: reachable challenger evidence min(e_max_rev, (D_max - delta_floor)*dt/(1 - lambda)) = "
      << compass::e_rev_reach_max(d_max, dt, k) << " does not exceed E0 = " << k.E0
      << " (D_max = w_g+w_s+w_e+w_r = " << d_max << ", delta_floor = " << k.delta_floor
      << ", lambda = " << k.lambda << ", e_max_rev = " << k.e_max_rev << ")";
  return msg.str();
}

}  // namespace compass_nav2

#endif  // COMPASS_NAV2__PARAM_CHECKS_HPP_
