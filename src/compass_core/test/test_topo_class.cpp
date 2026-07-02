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
