#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
#include "compass_core/accumulator.hpp"
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

template<class F> void rejects_invalid(F operation) {
  bool rejected = false;
  try { operation(); } catch (const std::invalid_argument&) { rejected = true; }
  assert(rejected);
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
    const auto r = run_safety_branch(sparse, safe, 0.5, 5.0, now, .05, k, tb);
    assert(r.mode != Mode::HOLD);
    assert(sparse.n_thrash == 1);
  }

  DecisionState dense;
  dense.c_star.set(7, Side::R);
  for (double now : {0.0, 0.1, 0.2}) {
    dense.c_star.set(7, Side::R);
    run_safety_branch(dense, safe, 0.5, 5.0, now, .05, k, tb);
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
  run_safety_branch(duplicate, safe, 0.5, 5.0, 2.0, .05, k, tb);
  run_safety_branch(duplicate, safe, 0.5, 5.0, 2.0, .05, k, tb);
  assert(duplicate.n_thrash == 1);
  bool rejected = false;
  try {
    run_safety_branch(duplicate, safe, 0.5, 5.0, 1.0, .05, k, tb);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  assert(rejected);

  // Issue #8: each genuine safety commitment starts a fresh progress epoch,
  // in measured and proxy modes. Stationary samples cannot revive old distance.
  for (bool measured : {false, true}) {
    DecisionCore epoch_core{Knobs{}};
    DecisionState epoch;
    epoch.c_star.set(7, Side::R);
    epoch.L_real = epoch.rho = 0.7;
    const auto delta = measured ? std::optional<double>{0.0} : std::nullopt;
    epoch_core.step_evals(safe, epoch, 5.0, 0.0, 1.0, 0.05, nullptr, delta);
    assert(epoch.c_star.equals(safe.front().cls));
    assert(epoch.rho == 0.0);
    assert(epoch.L_real == 0.0);
    epoch_core.step_evals(safe, epoch, 5.0, 0.0, 1.05, 0.05, nullptr, delta);
    assert(epoch.rho == 0.0);
  }

  // HOLD can re-commit without a preceding safety switch (threshold == 1).
  for (bool measured : {false, true}) {
    Knobs hold_knobs;
    hold_knobs.n_thrash = 1;
    DecisionCore hold_core{hold_knobs};
    DecisionState hold_epoch;
    hold_epoch.c_star.set(7, Side::R);
    hold_epoch.L_real = hold_epoch.rho = 0.7;
    const auto delta = measured ? std::optional<double>{0.0} : std::nullopt;
    hold_core.step_evals(safe, hold_epoch, 5.0, 0.0, 1.0, 0.05, nullptr, delta);
    assert(hold_epoch.mode == Mode::HOLD);
    assert(hold_epoch.c_star.equals(safe.front().cls));
    assert(hold_epoch.L_real == 0.0 && hold_epoch.rho == 0.0);
    hold_epoch.release_hold();
    hold_core.step_evals(safe, hold_epoch, 5.0, 0.0, 1.05, 0.05, nullptr, delta);
    assert(hold_epoch.L_real == 0.0 && hold_epoch.rho == 0.0);
  }

  // Braking without a class change must retain current measured progress.
  DecisionState braking_epoch;
  braking_epoch.c_star.set(7, Side::R);
  braking_epoch.L_real = braking_epoch.rho = 0.7;
  DecisionCore braking_core{Knobs{}};
  braking_core.step_evals({}, braking_epoch, 5.0, 0.0, 1.0, 0.05, nullptr, 0.0);
  assert(braking_epoch.L_real == 0.7 && braking_epoch.rho == 0.7);

  // Issue #6: invalid mathematical domains must be rejected, not silently
  // saturated into a plausible-looking threshold or reachability bound.
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  for (double invalid_lambda : {0.0, -0.1, 1.01, nan, inf}) {
    Knobs invalid;
    invalid.lambda = invalid_lambda;
    rejects_invalid([&] { DecisionCore bad{invalid}; });
    rejects_invalid([&] { e_rev_reach_max(1.0, .05, invalid); });
  }
  for (double invalid_positive : {0.0, -0.1, nan, inf}) {
    Knobs invalid;
    invalid.E0 = invalid_positive;
    rejects_invalid([&] { DecisionCore bad{invalid}; });
    DecisionState state;
    rejects_invalid([&] { accumulate(state, .5, invalid_positive, Knobs{}); });
    rejects_invalid([&] { braking_core.step_evals(evals, state, 5, 0, 1, invalid_positive); });
    rejects_invalid([&] { e_rev_reach_max(1, invalid_positive, Knobs{}); });
  }
  for (double invalid_nonnegative : {-0.1, nan, inf}) {
    for (double Knobs::* member : {&Knobs::e_max_rev, &Knobs::e_max_fwd,
        &Knobs::delta_floor, &Knobs::k_rho}) {
      Knobs invalid;
      invalid.*member = invalid_nonnegative;
      rejects_invalid([&] { DecisionCore bad{invalid}; });
    }
    Knobs invalid;
    invalid.a_brake = invalid_nonnegative;
    DecisionState state;
    rejects_invalid([&] { run_safety_branch(state, {}, .45, 5, 1, .05, invalid, tb); });
  }
  for (double invalid_advantage : {0.0, Knobs{}.delta_floor, nan, inf}) {
    rejects_invalid([&] { e_rev_reach_max(invalid_advantage, .05, Knobs{}); });
  }
  for (double invalid_rho : {-0.1, 1.01, nan, inf}) {
    rejects_invalid([&] { E_th(invalid_rho, Knobs{}); });
  }
  Knobs bad_exponent;
  bad_exponent.p = 0;
  rejects_invalid([&] { DecisionCore bad{bad_exponent}; });
  for (double rho : {0.0, .5, 1.0}) assert(E_th(rho, Knobs{}) >= Knobs{}.E0);
  Knobs no_hardening;
  no_hardening.k_rho = 0;
  assert(rho_bar(1, .05, no_hardening) == 1);
  assert(E_th(1, no_hardening) == no_hardening.E0);
  Knobs zero_caps;
  zero_caps.e_max_fwd = zero_caps.e_max_rev = 0;
  validate_accumulator_knobs(zero_caps);
  assert(e_rev_reach_max(1, .05, zero_caps) == 0);

  // Nine issue-#6 timing fixtures: 3/6/9-cycle lower bounds after each reset
  // kind. This is a discrete switching bound, not a human-legibility test.
  for (int minimum_cycles : {3, 6, 9}) {
    const double dt = .6 / minimum_cycles;
    for (int reset_kind : {0, 1, 2}) {
      Knobs timing_knobs;
      timing_knobs.lambda = 1;
      DecisionCore timing_core{timing_knobs};
      DecisionState timing;
      timing.c_star.set(7, Side::R);
      timing.e_rev = .29;
      double now = 1;
      if (reset_kind == 0) {
        // A changed challenger must discard the seeded evidence.
        TopoClass other;
        other.set(8, Side::L);
        timing.prev_c_prime = other;
      } else if (reset_kind == 1) {
        timing_core.step_evals(safe, timing, 5, 0, now, dt, nullptr, 0.0);
        assert(timing.e_rev == 0);
      } else {
        timing.e_rev = .3;
        timing.prev_c_prime = side(Side::L).cls;
        timing_core.step_evals({side(Side::R,.8),side(Side::L,.25)},
          timing, 5, 0, now, dt, nullptr, 0.0);
        assert(timing.c_star.equals(side(Side::L).cls));
        assert(timing.e_rev == 0);
      }
      const auto initial = timing.c_star;
      const auto current_side = initial.side(7).value();
      const auto challenger = current_side == Side::R ? Side::L : Side::R;
      int switch_cycle = 0;
      for (int cycle = 1; cycle <= minimum_cycles + 1; ++cycle) {
        now += dt;
        timing_core.step_evals({side(current_side,.8),side(challenger,.25)},
          timing, 5, 0, now, dt, nullptr, 0.0);
        if (!timing.c_star.equals(initial)) { switch_cycle = cycle; break; }
      }
      assert(switch_cycle >= minimum_cycles && switch_cycle <= minimum_cycles + 1);
    }
  }
  std::cout << "safety/domain contract: rolling HOLD, 5 progress epoch fixtures, "
    "invalid-domain rejection, 9 reset timing fixtures pass\n";
}
