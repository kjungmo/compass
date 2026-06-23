#include <gtest/gtest.h>
#include "compass_core/accumulator.hpp"
#include <cmath>
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

// research_spec §5.3 #2: 평균0·유계분산 D_k 입력에서 Δ_floor↑ 또는 E_0↑ 시
// 단위시간당 전환율이 단조 감소 (P1 지수 상한 경향).
// 결정적: 고정 시드 의사난수 D_k 시퀀스(외부 RNG 불사용)를 재사용하여
// margin 노브만 바꿔 전환 횟수를 비교한다.
TEST(Accumulator, P1SwitchRateDecreasesWithMargin) {
  // 고정 시드 LCG 로 평균 0 근방·유계 D_k 시퀀스 생성 (재현 가능).
  auto gen_seq = [](int n) {
    std::vector<double> seq;
    uint64_t st = 12345u;
    for (int i = 0; i < n; ++i) {
      st = st * 6364136223846793005ULL + 1442695040888963407ULL;
      double u = static_cast<double>((st >> 33) & 0xFFFFF) / static_cast<double>(0xFFFFF); // [0,1)
      seq.push_back((u - 0.5) * 0.6);   // 평균 ~0, 범위 ±0.3
    }
    return seq;
  };
  const int N = 20000;
  const double dt = 0.1;
  auto seq = gen_seq(N);

  // λ=1 순수 적분기로 비공허성 확보, 전환마다 리셋.
  auto count_switches = [&](double delta_floor, double E0) {
    Knobs k; k.lambda = 1.0; k.delta_floor = delta_floor; k.E0 = E0;
    DecisionState s;
    int switches = 0;
    for (double D : seq) {
      accumulate(s, D, dt, k);
      if (switch_ready(s, k)) { switches++; s.e_rev = 0; s.rho = 0; }
    }
    return switches;
  };

  // Δ_floor 증가 -> 전환율 단조 감소.
  int sw_low_floor  = count_switches(0.02, 0.30);
  int sw_high_floor = count_switches(0.10, 0.30);
  EXPECT_GT(sw_low_floor, sw_high_floor);

  // E_0 증가 -> 전환율 단조 감소.
  int sw_low_E0  = count_switches(0.05, 0.20);
  int sw_high_E0 = count_switches(0.05, 0.45);
  EXPECT_GT(sw_low_E0, sw_high_E0);
}

// 회귀: 기본 노브가 현실적 도전자에 대해 비공허해야 한다 (λ=0.97 재튜닝).
// 정상상태 증거: e_ss = (D-Δ_floor)·dt/(1-λ).  비공허 = e_ss > E0.
//   기본값 dt=0.05, E0=0.30, Δ_floor=0.05, λ=0.97:
//   D=0.30 -> e_ss = (0.30-0.05)*0.05/0.03 = 0.4167 > 0.30  => 전환 가능 (TRUE)
//   D=0.10 -> e_ss = (0.10-0.05)*0.05/0.03 = 0.0833 < 0.30  => 전환 불가 (FALSE)
// 구 기본값 λ=0.9 였다면 D=0.30 의 e_ss = 0.25*0.05/0.1 = 0.125 < 0.30 -> 아래 EXPECT_TRUE 가 실패.
TEST(Accumulator, DefaultKnobsNonVacuousForModerateChallenger) {
  Knobs k;  // 기본값 (λ=0.97 포함)
  EXPECT_TRUE(is_non_vacuous(/*D_max=*/0.30, /*dt=*/0.05, k));   // 중간 우위는 전환 가능
  EXPECT_FALSE(is_non_vacuous(/*D_max=*/0.10, /*dt=*/0.05, k));  // 약한 우위는 전환 불가
}
