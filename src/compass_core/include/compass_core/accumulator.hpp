// accumulator.hpp
#pragma once
#include "compass_core/knobs.hpp"
#include "compass_core/decision_state.hpp"
namespace compass {
// Throw std::invalid_argument for domains outside the discrete-time model:
// finite lambda in (0,1], E0>0, nonnegative caps/margins/hardening, p>=1,
// and a finite maximum threshold. k_rho=0 remains a supported ablation.
void validate_accumulator_knobs(const Knobs & k);
// 진행 경화 임계 E_th(rho) = E0 * (1 + k_rho * rho^p).
// rho must be finite and in [0,1]; therefore E_th(rho) >= E0.
double E_th(double rho, const Knobs & k);
// 분리 누적기 갱신: e_rev <- (D - Delta)dt, e_fwd <- (-D - Delta)dt, 각각 누수 + 클립.
// D must be finite and dt finite and positive. Lambda remains per-update, so
// changing dt changes the equivalent physical leak time; it is not rescaled here.
void accumulate(DecisionState & s, double D, double dt, const Knobs & k);
// 전환 준비 판정: e_rev >= E_th(rho).
bool switch_ready(const DecisionState & s, const Knobs & k);
// Positive-advantage reachability bound: finite D_max > delta_floor, dt > 0.
double e_rev_reach_max(double D_max, double dt, const Knobs & k);
// 비공허성 보장: 도달 가능 최대 > E0.
bool is_non_vacuous(double D_max, double dt, const Knobs & k);
// Progress-blocking boundary. 1 is a neutral sentinel when k_rho=0 (no
// progress-dependent blocking) or the base-threshold non-vacuity test fails;
// callers must check is_non_vacuous separately before interpreting a boundary.
double rho_bar(double D_max, double dt, const Knobs & k);
}  // namespace compass
