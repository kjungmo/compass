// accumulator.cpp (key bodies)
#include "compass_core/accumulator.hpp"
#include <algorithm>
#include <cmath>
namespace compass {
static double clip(double x, double lo, double hi){ return std::max(lo, std::min(hi, x)); }
double E_th(double rho, const Knobs& k){ return k.E0 * (1.0 + k.k_rho * std::pow(rho, k.p)); }
void accumulate(DecisionState& s, double D, double dt, const Knobs& k){
  s.e_rev = clip(k.lambda*s.e_rev + (D - k.delta_floor)*dt, 0.0, k.e_max_rev);
  s.e_fwd = clip(k.lambda*s.e_fwd + (-D - k.delta_floor)*dt, 0.0, k.e_max_fwd);
}
bool switch_ready(const DecisionState& s, const Knobs& k){ return s.e_rev >= E_th(s.rho, k); }
double e_rev_reach_max(double D_max, double dt, const Knobs& k){
  if (k.lambda >= 1.0) return k.e_max_rev;
  double eq = (D_max - k.delta_floor)*dt/(1.0 - k.lambda);
  return std::min(k.e_max_rev, eq);
}
bool is_non_vacuous(double D_max, double dt, const Knobs& k){ return e_rev_reach_max(D_max,dt,k) > k.E0; }
double rho_bar(double D_max, double dt, const Knobs& k){
  double reach = e_rev_reach_max(D_max, dt, k);
  if (reach <= k.E0) return 1.0;                      // never blocks within [0,1]
  double base = (reach/k.E0 - 1.0)/k.k_rho;
  return clip(std::pow(std::max(0.0, base), 1.0/k.p), 0.0, 1.0);
}
}  // namespace compass
