#pragma once
#include "compass_core/types.hpp"
#include <algorithm>
#include <cmath>
namespace compass_nav2 {
inline double applyCoreVelocityLimit(double cruise, const compass::DecisionOutput& out) {
  if (!std::isfinite(cruise) || cruise < 0 ||
      out.mode == compass::Mode::STOP || out.mode == compass::Mode::HOLD) return 0;
  if (out.safety_velocity_limited) {
    if (!std::isfinite(out.v_target)) return 0;
    return std::min(cruise, std::max(0., out.v_target));
  }
  // Preserve the existing measured-speed bootstrap for normal decisions.
  return out.v_target > .05 ? std::min(cruise, out.v_target) : cruise;
}
}
