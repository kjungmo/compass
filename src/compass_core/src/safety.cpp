// safety.cpp
#include "compass_core/safety.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace compass {

namespace {
// P5 counts the open-left window (now-W, now].
void prune_window(DecisionState & s, double now, double W) {
  while (!s.safe_switch_times.empty() && s.safe_switch_times.front() <= now - W) {
    s.safe_switch_times.pop_front();
  }
}
double decelerate(double v_in, const Knobs & k) {
  return std::max(0.0, v_in - k.a_brake);
}
}  // namespace

SafetyResult run_safety_branch(DecisionState & s,
                               const std::vector<ClassEval> & S_set,
                               double v_in, double ttc, double now,
                               const Knobs & k, TieBreaker & tb) {
  SafetyResult r;
  r.took_safety_branch = true;   // P3: 안전 분기가 재량 분기를 항상 선점

  // HOLD is absorbing until the caller explicitly releases DecisionState.
  if (s.mode == Mode::HOLD) {
    r.c_star = s.c_star;
    r.v_target = 0.0;
    r.velocity_limited = true;
    r.mode = Mode::HOLD;
    return r;
  }

  if (!std::isfinite(now) || !std::isfinite(k.W) || k.W <= 0.0)
    throw std::invalid_argument("safety window requires finite time and positive W");
  if (!s.safe_switch_times.empty() && now < s.safe_switch_times.back())
    throw std::invalid_argument("safety intervention time must be monotonic");

  // Record at most one qualifying intervention per decision time, then count
  // only events in the declared rolling window.
  if (s.safe_switch_times.empty() || now > s.safe_switch_times.back())
    s.safe_switch_times.push_back(now);
  prune_window(s, now, k.W);
  s.n_thrash = static_cast<int>(s.safe_switch_times.size());

  // 부분 임계: 창 내 안전 전환 수가 N_thrash 의 과반(>= N_thrash/2)에 도달하면
  // 1단계(전환)를 생략하고 감속 우선 (research_spec §3 step 2 partial_over).
  const int window_count = static_cast<int>(s.safe_switch_times.size());
  const int n_partial = std::max(1, k.n_thrash / 2);
  const bool partial_over = window_count >= n_partial;

  const bool in_dwell = now < s.t_safe_dwell;

  // 𝒮 중 안전 멤버만 후보로.
  std::vector<ClassEval> safe_set;
  for (const auto & c : S_set) if (c.safe) safe_set.push_back(c);

  if (in_dwell || partial_over) {
    // (i) 드웰 보호 또는 (ii) 부분 임계 초과 -> 2단계 감속, 필요 시 3단계 정지.
    r.c_star = s.c_star;            // 1단계(전환) 생략
    r.v_target = decelerate(v_in, k);
    r.velocity_limited = true;
    if (ttc < k.ttc_stop) { s.mode = Mode::STOP; }
  } else if (!safe_set.empty()) {
    // 1단계: 안전 전환.
    TopoClass picked = tb.pick(safe_set, s, k);
    s.c_star = picked;
    s.reset_on_safe_switch(now, k.T_safe_dwell);  // e_rev, rho 리셋 + 드웰 개시
    r.c_star = picked;
    r.v_target = v_in;             // 전환 시 속도는 유지 (감속 아님)
  } else {
    // 𝒮 비어있음 -> 감속, 필요 시 정지.
    r.c_star = s.c_star;
    r.v_target = decelerate(v_in, k);
    r.velocity_limited = true;
    if (ttc < k.ttc_stop) { s.mode = Mode::STOP; }
  }

  // 스래시 가드 (P5): N_thrash 도달 시 HOLD 강제.
  if (window_count >= k.n_thrash) {
    s.mode = Mode::HOLD;
    r.v_target = 0.0;
    r.velocity_limited = true;
  }

  r.mode = s.mode;
  return r;
}

}  // namespace compass
