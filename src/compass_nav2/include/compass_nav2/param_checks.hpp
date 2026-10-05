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

}  // namespace compass_nav2

#endif  // COMPASS_NAV2__PARAM_CHECKS_HPP_
