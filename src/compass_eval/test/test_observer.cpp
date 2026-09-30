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
  bool rejected = false;
  try { compute({}); } catch (const std::invalid_argument &) { rejected = true; }
  check(rejected);
}
