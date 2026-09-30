#include "compass_eval/harness.hpp"
#include <cstdlib>
#include <iostream>
using compass_eval::compute;
void check(bool ok) { if (!ok) { std::cerr << "observer regression failed\n"; std::exit(1); } }
int main() {
  for (int prefix : {30, 60, 100}) {
    std::vector<int> s(prefix, 1); s.resize(200, 0);
    auto m = compute(s);
    check(m.censored == (prefix == 100));
    if (!m.censored) check(std::abs(m.t_legible - (2 * prefix + 1) * 0.05) < 1e-9);
    for (auto & v : s) v = 1 - v;
    auto mirror = compute(s);
    check(mirror.censored == m.censored && std::abs(mirror.t_legible - m.t_legible) < 1e-9);
  }
  check(std::abs(compute(std::vector<int>(200, 1)).t_legible - 0.05) < 1e-9);
  // 99 R observations followed by 101 L observations cross only on the final
  // tick. No positive hold interval is observed, so the result is censored.
  std::vector<int> final_tick(99,1);final_tick.resize(200,0);
  check(compute(final_tick).censored);
  // Version audit (issue #3): the same streams under the archived rules.
  using compass_eval::ObserverRule;
  std::vector<int> r30(30, 1); r30.resize(200, 0);
  auto original = compute(r30, ObserverRule::ProbabilitySpace9fe495a);
  auto saturation_only = compute(r30, ObserverRule::LogOddsSuffix5343d45);
  check(original.censored);                       // reproduces the saturation defect
  check(!saturation_only.censored && std::abs(saturation_only.t_legible - 3.05) < 1e-9);
  // The saturation-only version accepts a final-sample crossing (t=9.95 s);
  // the current endpoint-suffix rule requires 0.30 s of observed follow-up.
  auto final_only = compute(final_tick, ObserverRule::LogOddsSuffix5343d45);
  check(!final_only.censored && std::abs(final_only.t_legible - 9.95) < 1e-9);
  // Exactly six follow-up intervals (0.30 s) after the first confident sample.
  std::vector<int> six(96, 1); six.resize(200, 0);   // crossing at index 193
  check(!compute(six).censored && std::abs(compute(six).t_legible - 9.65) < 1e-9);
  std::vector<int> five(97, 1); five.resize(200, 0); // crossing at index 195
  check(compute(five).censored && !compute(five, ObserverRule::LogOddsSuffix5343d45).censored);
  // Long hold before a reversal: no numerical saturation in log-odds space.
  std::vector<int> long_hold(80, 1); long_hold.resize(200, 0);
  check(!compute(long_hold).censored && std::abs(compute(long_hold).t_legible - 8.05) < 1e-9);
  bool rejected = false;
  try { compute({}); } catch (const std::invalid_argument &) { rejected = true; }
  check(rejected);
}
