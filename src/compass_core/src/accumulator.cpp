// accumulator.cpp (key bodies)
#include "compass_core/accumulator.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace compass {
static double clip(double x, double lo, double hi){ return std::max(lo, std::min(hi, x)); }
void validate_accumulator_knobs(const Knobs& k) {
  if (!std::isfinite(k.lambda) || k.lambda <= 0.0 || k.lambda > 1.0 ||
      !std::isfinite(k.E0) || k.E0 <= 0.0 ||
      !std::isfinite(k.e_max_rev) || k.e_max_rev < 0.0 ||
      !std::isfinite(k.e_max_fwd) || k.e_max_fwd < 0.0 ||
      !std::isfinite(k.delta_floor) || k.delta_floor < 0.0 ||
      !std::isfinite(k.k_rho) || k.k_rho < 0.0 || k.p < 1 ||
      !std::isfinite(k.E0 * (1.0 + k.k_rho)))
    throw std::invalid_argument("invalid discrete-time accumulator knobs");
}
double E_th(double rho, const Knobs& k){
  validate_accumulator_knobs(k);
  if (!std::isfinite(rho) || rho < 0.0 || rho > 1.0)
    throw std::invalid_argument("threshold requires rho in [0,1]");
  return k.E0 * (1.0 + k.k_rho * std::pow(rho, k.p));
}
void accumulate(DecisionState& s, double D, double dt, const Knobs& k){
  validate_accumulator_knobs(k);
  if (!std::isfinite(D) || !std::isfinite(dt) || dt <= 0.0)
    throw std::invalid_argument("accumulation requires finite advantage and positive dt");
  s.e_rev = clip(k.lambda*s.e_rev + (D - k.delta_floor)*dt, 0.0, k.e_max_rev);
  s.e_fwd = clip(k.lambda*s.e_fwd + (-D - k.delta_floor)*dt, 0.0, k.e_max_fwd);
}
bool switch_ready(const DecisionState& s, const Knobs& k){ return s.e_rev >= E_th(s.rho, k); }
double e_rev_reach_max(double D_max, double dt, const Knobs& k){
  validate_accumulator_knobs(k);
  if (!std::isfinite(D_max) || D_max <= k.delta_floor ||
      !std::isfinite(dt) || dt <= 0.0)
    throw std::invalid_argument("reachability requires D_max > delta_floor and positive dt");
  if (k.lambda == 1.0) return k.e_max_rev;
  double eq = (D_max - k.delta_floor)*dt/(1.0 - k.lambda);
  return std::min(k.e_max_rev, eq);
}
bool is_non_vacuous(double D_max, double dt, const Knobs& k){ return e_rev_reach_max(D_max,dt,k) > k.E0; }
double rho_bar(double D_max, double dt, const Knobs& k){
  double reach = e_rev_reach_max(D_max, dt, k);
  if (reach <= k.E0 || k.k_rho == 0.0) return 1.0;    // documented neutral sentinel
  double base = (reach/k.E0 - 1.0)/k.k_rho;
  return clip(std::pow(std::max(0.0, base), 1.0/k.p), 0.0, 1.0);
}
}  // namespace compass
