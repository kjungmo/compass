#include <cassert>
#include <stdexcept>
#include <vector>
#include "compass_core/decision_core.hpp"
#include "compass_core/safety.hpp"

using namespace compass;

namespace {
ClassEval side(Side s, double cost = 0.2) {
  ClassEval e;
  e.cls.set(7, s);
  e.J = cost;
  e.available = true;
  e.safe = true;
  return e;
}
}  // namespace

int main() {
  Knobs k;
  k.W = 3.0;
  k.n_thrash = 3;
  DecisionState sparse;
  sparse.c_star.set(7, Side::R);
  TieBreaker tb;
  const std::vector<ClassEval> safe = {side(Side::L)};
  for (double now : {0.0, 3.0, 6.0, 9.0}) {
    sparse.c_star.set(7, Side::R);
    const auto r = run_safety_branch(sparse, safe, 0.5, 5.0, now, k, tb);
    assert(r.mode != Mode::HOLD);
    assert(sparse.n_thrash == 1);
  }

  DecisionState dense;
  dense.c_star.set(7, Side::R);
  for (double now : {0.0, 0.1, 0.2}) {
    dense.c_star.set(7, Side::R);
    run_safety_branch(dense, safe, 0.5, 5.0, now, k, tb);
  }
  assert(dense.mode == Mode::HOLD);
  assert(dense.n_thrash == 3);

  DecisionCore core(k);
  const std::vector<ClassEval> evals = {side(Side::R, 0.5), side(Side::L, 0.1)};
  const auto held = core.step_evals(evals, dense, 5.0, 0.8, 10.0, 0.05);
  assert(held.mode == Mode::HOLD);
  assert(held.v_target == 0.0);
  assert(held.safety_velocity_limited);

  dense.release_hold();
  assert(dense.mode == Mode::NORMAL);
  assert(dense.n_thrash == 0);
  assert(dense.safe_switch_times.empty());
  const auto released = core.step_evals(evals, dense, 5.0, 0.8, 10.05, 0.05);
  assert(released.mode == Mode::NORMAL);
  assert(released.v_target > 0.0);

  DecisionState duplicate;
  duplicate.c_star.set(7, Side::R);
  run_safety_branch(duplicate, safe, 0.5, 5.0, 2.0, k, tb);
  run_safety_branch(duplicate, safe, 0.5, 5.0, 2.0, k, tb);
  assert(duplicate.n_thrash == 1);
  bool rejected = false;
  try {
    run_safety_branch(duplicate, safe, 0.5, 5.0, 1.0, k, tb);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  assert(rejected);
}
