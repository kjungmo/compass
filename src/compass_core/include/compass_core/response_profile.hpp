#pragma once
#include "compass_core/knobs.hpp"
#include <cmath>
#include <optional>
namespace compass {
// Opt-in research profile. P4 permanent blocking is NOT claimed for this profile.
inline Knobs responsive_profile() { Knobs k; k.k_rho = 0.5; return k; }
// Worst-rho bound for a fixed challenger with D >= sustained_advantage every cycle,
// both classes safe, no resets, zero initial evidence, constant dt and valid knobs.
// Equality at the cap is reachable; equality at a leaky equilibrium is not.
inline std::optional<unsigned> response_cycle_bound(double D, double dt, const Knobs& k) {
  const double E = k.E0 * (1 + k.k_rho);
  if (!std::isfinite(D) || !std::isfinite(dt) || dt <= 0 ||
      !std::isfinite(k.E0) || k.E0 <= 0 || !std::isfinite(k.k_rho) || k.k_rho < 0 ||
      k.p < 1 || !std::isfinite(k.lambda) || k.lambda <= 0 || k.lambda > 1 ||
      !std::isfinite(k.e_max_rev) || !std::isfinite(k.delta_floor) ||
      k.delta_floor < 0 || !std::isfinite(E) || E > k.e_max_rev) return std::nullopt;
  const double a = (D-k.delta_floor)*dt;
  if (!std::isfinite(a) || a <= 0) return std::nullopt;
  double n;
  if (k.lambda == 1) n = E/a;
  else {
    const double equilibrium = a/(1-k.lambda);
    if (equilibrium <= E) return std::nullopt;
    n = std::log1p(-E/equilibrium)/std::log(k.lambda);
  }
  if (!std::isfinite(n) || n > 1000000000) return std::nullopt;
  // One conservative extra cycle avoids rounding down at exact integer crossings.
  return static_cast<unsigned>(std::ceil(n)) + 1;
}
}
