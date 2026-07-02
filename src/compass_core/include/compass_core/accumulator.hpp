// accumulator.hpp
#pragma once
#include "compass_core/knobs.hpp"
#include "compass_core/decision_state.hpp"
namespace compass {
// 진행 경화 임계 E_th(rho) = E0 * (1 + k_rho * rho^p).
double E_th(double rho, const Knobs & k);
// 분리 누적기 갱신: e_rev <- (D - Delta)dt, e_fwd <- (-D - Delta)dt, 각각 누수 + 클립.
void accumulate(DecisionState & s, double D, double dt, const Knobs & k);
// 전환 준비 판정: e_rev >= E_th(rho).
bool switch_ready(const DecisionState & s, const Knobs & k);
// 정상상태에서 D_max 일 때 e_rev 가 도달 가능한 최대값.
double e_rev_reach_max(double D_max, double dt, const Knobs & k);
// 비공허성 보장: 도달 가능 최대 > E0.
bool is_non_vacuous(double D_max, double dt, const Knobs & k);
// 봉쇄 진행도 상한 rho_bar.
double rho_bar(double D_max, double dt, const Knobs & k);
}  // namespace compass
