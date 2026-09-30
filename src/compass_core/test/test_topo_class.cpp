#include <gtest/gtest.h>
#include "compass_core/topo_class.hpp"
using compass::TopoClass; using compass::Side;
TEST(TopoClass, EqualsAndHamming) {
  TopoClass a, b;
  a.set(7, Side::R); a.set(12, Side::L);
  b.set(7, Side::R); b.set(12, Side::R);
  EXPECT_TRUE(a.equals(a));
  EXPECT_FALSE(a.equals(b));
  EXPECT_EQ(TopoClass::hamming(a, b), 1);
}
TEST(TopoClass, LexOrderIdAscThenLbeforeR) {
  TopoClass a, b;            // same ids, a has L at 7, b has R at 7
  a.set(7, Side::L); a.set(12, Side::R);
  b.set(7, Side::R); b.set(12, Side::R);
  EXPECT_TRUE(TopoClass::lex_less(a, b));   // L < R at first differing id
}
TEST(TopoClass, RestrictRemovesDimension) {
  TopoClass a; a.set(7, Side::R); a.set(12, Side::L);
  TopoClass r = a.restrict({12});
  EXPECT_EQ(r.size(), 1u);
  EXPECT_EQ(r.side(7).value(), Side::R);
  EXPECT_FALSE(r.side(12).has_value());
}

// Issue #8: one steering side-bias definition shared by the tracker and L_plan.
TEST(TopoClass, SideBiasIsMeanPairSign) {
  TopoClass none;
  EXPECT_DOUBLE_EQ(compass::side_bias(none), 0.0);                  // empty: no bias
  TopoClass l; l.set(1, Side::L); l.set(2, Side::L); l.set(3, Side::L);
  EXPECT_DOUBLE_EQ(compass::side_bias(l), 1.0);                     // all left
  TopoClass r; r.set(1, Side::R); r.set(2, Side::R);
  EXPECT_DOUBLE_EQ(compass::side_bias(r), -1.0);                    // all right
  TopoClass m; m.set(1, Side::L); m.set(2, Side::L); m.set(3, Side::R);
  EXPECT_DOUBLE_EQ(compass::side_bias(m), 1.0 / 3.0);               // mixed 2L+1R
  TopoClass b; b.set(1, Side::L); b.set(2, Side::R);
  EXPECT_DOUBLE_EQ(compass::side_bias(b), 0.0);                     // balanced 1L+1R cancels
}
