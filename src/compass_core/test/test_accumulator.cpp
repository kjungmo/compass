#include <gtest/gtest.h>
#include "compass_core/accumulator.hpp"
using namespace compass;
TEST(Accumulator, P1StableUnderFloor) {        // |D_k| <= Delta_floor => e_rev stays 0, never switches
  Knobs k; DecisionState s;
  for (int i = 0; i < 1000; ++i) {
    double D = (i % 2 == 0 ? 1 : -1) * (k.delta_floor * 0.5);  // |D| < Delta_floor
    accumulate(s, D, 0.05, k);
    ASSERT_FALSE(switch_ready(s, k));
  }
  EXPECT_DOUBLE_EQ(s.e_rev, 0.0);
}
TEST(Accumulator, NonVacuityGuardDetectsInertConfig) {
  // research_spec §4.5: e^rev_max = min(e_max_rev, (D_max - Δ_floor)·dt / (1 - λ)).
  // Inert config: heavy leak (λ small) + small dt => reach < E0 => no discretionary switch possible.
  Knobs k; k.lambda = 0.7;                       // reach = (0.5-0.05)*0.05/0.3 = 0.075 < E0=0.3
  EXPECT_FALSE(is_non_vacuous(/*D_max=*/0.5, /*dt=*/0.05, k));
  // Defaults (λ=0.9) become non-vacuous once the equilibrium term clears E0:
  // reach = min(0.5, (0.8-0.05)*0.1/0.1) = min(0.5, 0.75) = 0.5 > E0=0.3.
  Knobs k2;
  EXPECT_TRUE(is_non_vacuous(/*D_max=*/0.8, /*dt=*/0.1, k2));
}
TEST(Accumulator, HeadroomConditionHolds) {
  Knobs k;
  EXPECT_GT(k.e_max_fwd, E_th(1.0, k));          // e_max_fwd > max_rho E_th
}
TEST(Accumulator, EthMonotoneInRho) {
  Knobs k;
  EXPECT_LT(E_th(0.0, k), E_th(0.5, k));
  EXPECT_LT(E_th(0.5, k), E_th(1.0, k));
}
