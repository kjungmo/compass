#include <gtest/gtest.h>
#include "compass_core/decision_state.hpp"
#include "compass_core/knobs.hpp"
using namespace compass;
TEST(Knobs, DefaultsMatchSpec) {
  Knobs k;
  EXPECT_DOUBLE_EQ(k.delta_floor, 0.05);
  EXPECT_DOUBLE_EQ(k.E0, 0.30);
  EXPECT_GT(k.e_max_fwd, k.E0 * (1 + k.k_rho));   // O4 headroom
  EXPECT_GT(k.e_max_rev, k.E0);                    // non-vacuity
  EXPECT_LE(k.e_max_rev, k.E0 * (1 + k.k_rho));    // P4 blocking band
}
TEST(DecisionState, ResetOnCommitClearsRevRhoFwd) {
  DecisionState s; s.e_fwd=0.4; s.e_rev=0.4; s.rho=0.7; s.L_real=2.0;
  s.reset_on_commit();
  EXPECT_DOUBLE_EQ(s.e_rev, 0.0); EXPECT_DOUBLE_EQ(s.rho, 0.0);
  EXPECT_DOUBLE_EQ(s.L_real, 0.0); EXPECT_DOUBLE_EQ(s.e_fwd, 0.0);
}
