#include <gtest/gtest.h>
#include "compass_core/safety.hpp"
#include "compass_core/tie_breaker.hpp"
using namespace compass;

static ClassEval mk(std::initializer_list<std::pair<uint64_t, Side>> v, double J,
                    bool safe = true) {
  ClassEval e;
  for (auto & p : v) e.cls.set(p.first, p.second);
  e.J = J; e.available = safe; e.safe = safe;
  return e;
}

// research_spec §5.3 #5: c* ∉ 𝒮 이면 e_rev 가 임계 위여도 안전 사다리가 선점 (P3).
TEST(Safety, P3SafetyDominatesAccumulator) {
  Knobs k; DecisionState s; s.c_star.set(7, Side::R);
  s.e_rev = 10.0;   // 임계를 한참 넘긴 누적기 (재량 전환이 발화할 법한 상태)
  TieBreaker tb;
  // 안전한 대체 class 가 존재 -> 안전 사다리는 전환을 수행하되, 재량 분기가 아니라
  // 안전 분기에서 일어났음을 표시.
  std::vector<ClassEval> S = { mk({{7, Side::L}}, 0.20) };
  SafetyResult r = run_safety_branch(s, S, /*v_in=*/0.5, /*ttc=*/5.0, /*now=*/1.0, k, tb);
  EXPECT_TRUE(r.took_safety_branch);                 // 안전 분기 선점
  EXPECT_TRUE(r.c_star.equals(mk({{7, Side::L}}, 0).cls));  // 안전 전환됨
  EXPECT_DOUBLE_EQ(s.e_rev, 0.0);                    // 안전 전환 -> e_rev 리셋
  EXPECT_DOUBLE_EQ(s.rho, 0.0);
}

// research_spec §5.3 #6: 드웰 보호 중에는 1단계(전환) 생략, 감속->정지 단조 하강.
TEST(Safety, SafetyLadderMonotone) {
  Knobs k; DecisionState s; s.c_star.set(7, Side::R);
  s.t_safe_dwell = 100.0;   // 드웰 보호 활성 (now < t_safe_dwell)
  TieBreaker tb;
  std::vector<ClassEval> S = { mk({{7, Side::L}}, 0.20) };  // 안전 대체 존재
  double v = 1.0;
  double prev = v;
  // 드웰 동안 반복 호출: 전환하지 않고 감속만, ttc 충분 -> STOP 아님.
  // (n_thrash HOLD 마스킹을 피하려고 호출 수를 N_thrash 미만으로 제한.)
  const int iters = std::min(3, k.n_thrash - 1);
  for (int i = 0; i < iters; ++i) {
    SafetyResult r = run_safety_branch(s, S, v, /*ttc=*/5.0, /*now=*/1.0, k, tb);
    EXPECT_TRUE(r.c_star.equals(s.c_star));     // class 전환 안 함 (1단계 생략)
    EXPECT_LE(r.v_target, prev);                // 단조 비증가
    EXPECT_NE(r.mode, Mode::HOLD);
    prev = r.v_target; v = r.v_target;
  }
  EXPECT_LT(prev, 1.0);                          // 실제로 감속됨

  // ttc 가 ttc_stop 미만이면 STOP 으로 전이 (신선한 상태로 검증, HOLD 마스킹 회피).
  DecisionState s2; s2.c_star.set(7, Side::R); s2.t_safe_dwell = 100.0;
  TieBreaker tb2;
  SafetyResult rs = run_safety_branch(s2, S, 0.5, /*ttc=*/k.ttc_stop * 0.5, 1.0, k, tb2);
  EXPECT_EQ(rs.mode, Mode::STOP);
  EXPECT_LT(rs.v_target, 0.5);                   // 정지 직전에도 감속 단조 적용
}

// research_spec §5.3 #7: 창 W 내 안전 전환이 N_thrash 도달 시 HOLD (P5).
TEST(Safety, P5ThrashGuardHolds) {
  Knobs k; DecisionState s; s.c_star.set(7, Side::R);
  TieBreaker tb;
  std::vector<ClassEval> S = { mk({{7, Side::L}}, 0.20) };
  Mode final_mode = Mode::NORMAL;
  // c* 가 매번 안전 집합 밖이 되도록 c_star 를 S 밖으로 강제하며 반복.
  for (int i = 0; i < k.n_thrash + 2; ++i) {
    s.c_star.set(7, Side::R);   // S 는 {7L} 뿐이므로 c_star=7R 는 늘 ∉ S
    SafetyResult r = run_safety_branch(s, S, 0.5, /*ttc=*/5.0, /*now=*/0.1 * i, k, tb);
    final_mode = r.mode;
  }
  EXPECT_EQ(final_mode, Mode::HOLD);
  EXPECT_GE(s.n_thrash, k.n_thrash);
}
