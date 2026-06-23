#include <gtest/gtest.h>
#include "compass_core/tie_breaker.hpp"
using namespace compass;

static ClassEval mk(std::initializer_list<std::pair<uint64_t, Side>> v, double J) {
  ClassEval e;
  for (auto & p : v) e.cls.set(p.first, p.second);
  e.J = J; e.available = true; e.safe = true;
  return e;
}

// research_spec §5.3 #8: 동률 도전자 집합에서 결정적 선택 (동일 입력 => 동일 c′).
TEST(TieBreaker, DeterministicUnderTie) {
  Knobs k; DecisionState st; st.c_star.set(7, Side::R);
  std::vector<ClassEval> ch = {
    mk({{7, Side::L}}, 0.50),
    mk({{7, Side::R}, {9, Side::L}}, 0.50 + 1e-9)
  };
  TieBreaker tb;
  TopoClass a = tb.pick(ch, st, k);
  TieBreaker tb2;
  TopoClass b = tb2.pick(ch, st, k);
  EXPECT_TRUE(a.equals(b));   // same input => same pick
}

// 우선순위 검증: prev_c_prime 가 동률 집합 안에 있으면 우선 (R6 규칙 1).
TEST(TieBreaker, PrevChallengerWinsTie) {
  Knobs k; DecisionState st; st.c_star.set(7, Side::R);
  TopoClass prev; prev.set(9, Side::R);
  st.prev_c_prime = prev;
  std::vector<ClassEval> ch = {
    mk({{7, Side::L}}, 0.50),       // smaller hamming to c_star? hamming({7L},{7R})=1
    mk({{9, Side::R}}, 0.50 + 1e-4) // = prev_c_prime, within eps_out
  };
  TieBreaker tb;
  TopoClass w = tb.pick(ch, st, k);
  EXPECT_TRUE(w.equals(prev));   // prev_c_prime wins despite slightly worse J
}

// 해밍 우선 (R6 규칙 2): prev 없으면 c* 와의 해밍 거리 작은 쪽.
TEST(TieBreaker, HammingTieBreakWhenNoPrev) {
  Knobs k; DecisionState st; st.c_star.set(7, Side::R); st.c_star.set(9, Side::L);
  std::vector<ClassEval> ch = {
    mk({{7, Side::L}, {9, Side::L}}, 0.50),  // hamming to c_star = 1
    mk({{7, Side::L}, {9, Side::R}}, 0.50)   // hamming to c_star = 2
  };
  TieBreaker tb;
  TopoClass w = tb.pick(ch, st, k);
  EXPECT_EQ(TopoClass::hamming(w, st.c_star), 1);
}

// research_spec §5.3 #9: 경계 채터링 봉쇄 (ε_in < ε_out 히스테리시스).
TEST(TieBreaker, BoundaryHysteresisNoChatter) {
  Knobs k; DecisionState st; st.c_star.set(7, Side::R);
  TieBreaker tb;
  TopoClass last; bool first = true; int flips = 0;
  for (int i = 0; i < 50; ++i) {
    double j2 = 0.50 + ((i % 2) ? k.eps_in * 0.5 : -k.eps_in * 0.5);
    std::vector<ClassEval> ch = {
      mk({{7, Side::L}}, 0.50),
      mk({{9, Side::R}}, j2)
    };
    TopoClass w = tb.pick(ch, st, k);
    if (!first && !w.equals(last)) flips++;
    last = w; first = false;
  }
  EXPECT_LE(flips, 1);
}
