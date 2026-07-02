// harness.hpp — COMPASS 오프라인 실험 하니스 (header-only).
// 재현 가능한 조우 시나리오(스크립트 D_k)로 실제 결정 코어를 구동하고,
// 각 ablation 변형의 행동 일관성 지표를 산출한다. 측정값 날조 없음 —
// 모든 수치는 결정 스트림에서 결정적으로 계산된다.
#pragma once
#include <vector>
#include <random>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include "compass_core/decision_core.hpp"
#include "compass_core/accumulator.hpp"

namespace compass_eval {
using namespace compass;

constexpr uint64_t PID = 7;     // 단일 보행자 ID
constexpr double DT = 0.05;     // 20 Hz 결정 주기 (기본 노브 정합)
constexpr int T = 200;          // 10 s 조우

// 한 주기의 비용/안전 신호. adv = J_R - J_L (양수 => L 이 더 저렴 => L 우위).
struct Cycle { double J_L, J_R, ttc, v_in; bool safe_L, safe_R; };

inline ClassEval mk(Side s, double J, bool safe) {
  ClassEval e; e.cls.set(PID, s); e.J = J; e.available = safe; e.safe = safe; return e;
}

// ---------------------------------------------------------------- 시나리오
enum class Scn { NearTie, Spike, Reversal, CleanCommit, Intermittent };
inline const char * scn_name(Scn s) {
  switch (s) {
    case Scn::NearTie:      return "near_tie";
    case Scn::Spike:        return "transient_spike";
    case Scn::Reversal:     return "mid_reversal";
    case Scn::CleanCommit:  return "clean_commit";
    default:                return "intermittent";
  }
}
inline std::vector<Scn> all_scns() {
  return {Scn::NearTie, Scn::Spike, Scn::Reversal, Scn::CleanCommit, Scn::Intermittent};
}

// c* 는 R 에서 출발. adv(i) 신호 + 가우시안 잡음으로 J_L, J_R 생성.
// v_in 은 ρ 를 구동하는 횡 기동 진행률. ρ 는 횡(cross-track) 진행도이므로
// 순항 0.5 m/s 의 횡 성분(소각 회피 ≈0.1배)을 반영해 v_lat=0.05 로 둔다.
// (코어의 v_lat=|v_cmd| 단순화를 실험에서 보정; L_plan=1.0 m, ρ 는 조우 동안 0→~0.5.)
inline std::vector<Cycle> make_scenario(Scn s, uint32_t seed, double vlat = 0.05) {
  std::mt19937 rng(seed);
  std::vector<Cycle> out; out.reserve(T);
  for (int i = 0; i < T; ++i) {
    double adv = 0.0, sigma = 0.03;
    switch (s) {
      case Scn::NearTie:     adv = 0.0;  sigma = 0.12; break;           // 모호한 동률
      case Scn::Spike:       adv = (i >= 80 && i < 100) ? +0.25 : -0.25; break;  // 일시 스파이크
      case Scn::Reversal:    adv = -0.25 + 0.50 * (double(i) / (T - 1)); break;  // 중간 역전
      case Scn::CleanCommit: adv = +0.40; break;                        // 명확한 L 우위
      case Scn::Intermittent: { int ph = i % 12; adv = (ph < 8) ? +0.45 : -0.05; } break; // 간헐 우위
    }
    std::normal_distribution<double> nd(0.0, sigma);
    adv += nd(rng);
    Cycle c;
    c.J_R = 0.50;
    c.J_L = c.J_R - adv;   // adv>0 => J_L<J_R
    c.ttc = 5.0; c.v_in = vlat; c.safe_L = true; c.safe_R = true;
    out.push_back(c);
  }
  return out;
}

// ---------------------------------------------------------------- 변형(변종)
enum class Var { Full, NoHyst, NoHard, Argmin, Dwell, NoClass };
inline const char * var_name(Var v) {
  switch (v) {
    case Var::Full:    return "full (제안)";
    case Var::NoHyst:  return "-hysteresis";
    case Var::NoHard:  return "-progress hardening";
    case Var::Argmin:  return "-accumulator (즉시 argmin)";
    case Var::Dwell:   return "simple-dwell (O4)";
    default:           return "-class correspondence";
  }
}
inline std::vector<Var> all_vars() {
  return {Var::Full, Var::NoHyst, Var::NoHard, Var::Argmin, Var::Dwell, Var::NoClass};
}

// 실제 결정 코어 구동 (full / -hysteresis / -hardening 은 노브만 다름).
inline std::vector<int> run_core(const std::vector<Cycle> & sc, Knobs k) {
  DecisionCore core(k);
  DecisionState st; st.c_star.set(PID, Side::R); st.L_plan = 1.0;
  std::vector<int> out; out.reserve(sc.size());
  for (size_t i = 0; i < sc.size(); ++i) {
    std::vector<ClassEval> evals = { mk(Side::R, sc[i].J_R, sc[i].safe_R),
                                     mk(Side::L, sc[i].J_L, sc[i].safe_L) };
    DecisionOutput o = core.step_evals(evals, st, sc[i].ttc, sc[i].v_in, i * DT, DT);
    out.push_back(o.c_star.side(PID).value() == Side::R ? 1 : 0);
  }
  return out;
}

// -accumulator: 매 주기 안전 class 중 argmin J 즉시 커밋 (누수 누적기 제거).
inline std::vector<int> run_argmin(const std::vector<Cycle> & sc) {
  int cur = 1; std::vector<int> out; out.reserve(sc.size());
  for (const auto & c : sc) {
    if (c.safe_L && c.safe_R) {
      if (c.J_L < c.J_R - 1e-9) cur = 0;
      else if (c.J_R < c.J_L - 1e-9) cur = 1;   // 정확 동률은 현 상태 유지
    } else if (c.safe_L) cur = 0; else if (c.safe_R) cur = 1;
    out.push_back(cur);
  }
  return out;
}

// -class correspondence: per-ID class 추적 상실. 근접 동률대(|ΔJ|<band)에서
// 정체성 모호로 무작위 깜빡임, 그 외에는 argmin. (정체 깜빡임 모델)
inline std::vector<int> run_noclass(const std::vector<Cycle> & sc, uint32_t seed) {
  int cur = 1; std::mt19937 rng(seed); std::uniform_int_distribution<int> coin(0, 1);
  const double band = 0.10;
  std::vector<int> out; out.reserve(sc.size());
  for (const auto & c : sc) {
    if (c.safe_L && c.safe_R) {
      if (std::abs(c.J_R - c.J_L) < band) cur = coin(rng);
      else cur = (c.J_L < c.J_R) ? 0 : 1;
    } else if (c.safe_L) cur = 0; else if (c.safe_R) cur = 1;
    out.push_back(cur);
  }
  return out;
}

// simple-dwell (O4): 누수 누적 대신 연속 드웰 타이머. 도전자가 T_dwell 동안
// 연속으로 우위여야 전환; 한 번이라도 끊기면 리셋(간헐 우위에서 교착).
inline std::vector<int> run_dwell(const std::vector<Cycle> & sc) {
  int cur = 1, lead = 1, dwell = 0;
  const int TD = static_cast<int>(std::lround(0.6 / DT));   // T_dwell = 0.6 s = 12 주기
  std::vector<int> out; out.reserve(sc.size());
  for (const auto & c : sc) {
    int best;
    if (c.safe_L && c.safe_R) best = (c.J_L < c.J_R) ? 0 : 1;
    else if (c.safe_L) best = 0; else best = 1;
    if (best == cur) { dwell = 0; lead = cur; }
    else {
      if (best == lead) ++dwell; else { lead = best; dwell = 1; }
      if (dwell >= TD) { cur = best; dwell = 0; lead = cur; }
    }
    out.push_back(cur);
  }
  return out;
}

inline std::vector<int> run_variant(Var v, const std::vector<Cycle> & sc, uint32_t seed) {
  switch (v) {
    case Var::Full:   { Knobs k;                   return run_core(sc, k); }
    case Var::NoHyst: { Knobs k; k.delta_floor = 0; return run_core(sc, k); }
    case Var::NoHard: { Knobs k; k.k_rho = 0;       return run_core(sc, k); }
    case Var::Argmin: return run_argmin(sc);
    case Var::Dwell:  return run_dwell(sc);
    default:          return run_noclass(sc, seed);
  }
}

// ---------------------------------------------------------------- 지표
struct Metrics {
  int switches = 0;
  double sign_change_rate = 0;   // 초당 부호 변화
  double entropy = 0;            // 결정 엔트로피 (bits/decision)
  double t_legible = 0;          // time-to-legible (s)
  bool censored = false;         // 사후확률 미안정 -> 우측 검열
};

// §4.7 베이즈 관찰자: 외부 관측 가능한 통과 측 스트림으로 사후확률을 갱신,
// 임계 p* 를 넘어 끝까지 유지되는 최초 시각을 time-to-legible 로 둔다.
inline Metrics compute(const std::vector<int> & sides) {
  const int n = static_cast<int>(sides.size());
  int sw = 0;
  for (int i = 1; i < n; ++i) if (sides[i] != sides[i - 1]) ++sw;
  Metrics m;
  m.switches = sw;
  m.sign_change_rate = sw / (n * DT);
  const double psw = (n > 1) ? double(sw) / (n - 1) : 0.0;
  m.entropy = (psw > 0 && psw < 1) ? -psw * std::log2(psw) - (1 - psw) * std::log2(1 - psw) : 0.0;

  const int Hstar = sides[n - 1];
  const double eps = 0.2, pstar = 0.9;
  double postR = 0.5;
  std::vector<double> postH(n);
  for (int i = 0; i < n; ++i) {
    const double lik_R = (sides[i] == 1) ? (1 - eps) : eps;
    const double lik_L = (sides[i] == 0) ? (1 - eps) : eps;
    const double a = postR * lik_R, b = (1 - postR) * lik_L;
    postR = a / (a + b);
    postH[i] = (Hstar == 1) ? postR : (1 - postR);
  }
  int t_idx = -1;
  for (int i = n - 1; i >= 0; --i) { if (postH[i] >= pstar) t_idx = i; else break; }
  if (t_idx < 0) { m.t_legible = n * DT; m.censored = true; }
  else           { m.t_legible = t_idx * DT; m.censored = false; }
  return m;
}

}  // namespace compass_eval
