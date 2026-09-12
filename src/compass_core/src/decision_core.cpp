// decision_core.cpp
#include "compass_core/decision_core.hpp"
#include "compass_core/accumulator.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace compass {

DecisionCore::DecisionCore(Knobs k) : k_(k), cost_(k) {}

std::vector<TopoClass> DecisionCore::enumerate(const DecisionInput & in) const {
  // 상위 K_cap 명만 명시 차원으로. (단순화: 입력 순서 상위 K_cap.)
  const int K = std::min(static_cast<int>(in.people.size()), k_.K_cap);
  std::vector<TopoClass> classes;
  if (K == 0) { classes.emplace_back(); return classes; }   // 빈 class 하나
  const int combos = 1 << K;   // 2^K
  for (int mask = 0; mask < combos; ++mask) {
    TopoClass c;
    for (int i = 0; i < K; ++i) {
      const Side s = (mask & (1 << i)) ? Side::R : Side::L;
      c.set(in.people[i].id, s);
    }
    classes.push_back(c);
  }
  return classes;
}

DecisionOutput DecisionCore::step(const DecisionInput & in, DecisionState & st,
                                  const IEnvQuery & env) {
  return step(in, st, env, nullptr);
}

DecisionOutput DecisionCore::step(const DecisionInput & in, DecisionState & st,
                                  const IEnvQuery & env, DecisionTrace * trace) {
  // 1) 열거 + 비용 + 안전 집합.
  std::vector<TopoClass> classes = enumerate(in);
  std::vector<ClassEval> evals;
  evals.reserve(classes.size());
  for (const auto & c : classes) {
    ClassEval e;
    e.cls = c;
    e.J = cost_.J(c, in, env);
    e.available = std::isfinite(e.J);
    e.safe = e.available &&
             env.clearance(c) >= k_.d_safe &&
             env.ttc(c) >= k_.ttc_min &&
             env.feasible(c);
    evals.push_back(e);
  }
  // ttc(c*) — 현재 커밋 class 의 TTC (안전 사다리 입력).
  const double ttc_cstar = env.ttc(st.c_star);
  const double v_in = in.robot_vel.vx;
  return step_evals(evals, st, ttc_cstar, v_in, in.now, in.dt, trace, in.lateral_progress_delta_m);
}

DecisionOutput DecisionCore::step_evals(const std::vector<ClassEval> & evals,
                                        DecisionState & st, double ttc,
                                        double v_in, double now, double dt) {
  return step_evals(evals, st, ttc, v_in, now, dt, nullptr);
}

DecisionOutput DecisionCore::step_evals(const std::vector<ClassEval> & evals,
    DecisionState & st, double ttc, double v_in, double now, double dt,
    DecisionTrace * trace) {
  return step_evals(evals, st, ttc, v_in, now, dt, trace, std::nullopt);
}

DecisionOutput DecisionCore::step_evals(const std::vector<ClassEval> & evals,
    DecisionState & st, double ttc, double v_in, double now, double dt,
    DecisionTrace * trace, std::optional<double> progress_delta_m) {
  if (progress_delta_m && (!std::isfinite(*progress_delta_m) ||
      !std::isfinite(dt) || dt <= 0))
    throw std::invalid_argument("measured progress requires finite delta and positive dt");
  bool discretionary_commit = false;
  if (trace) {
    *trace = DecisionTrace{};
    trace->measured_progress_delta_m = progress_delta_m;
    trace->now = now; trace->dt = dt; trace->v_input = v_in;
    trace->before = st; trace->candidates = evals;
  }
  auto finalize_trace = [&](const DecisionOutput & result) {
    if (trace) {
      trace->after = st; trace->v_output = result.v_target;
      trace->switched = !trace->before.c_star.equals(st.c_star);
    }
  };
  DecisionOutput out;
  double v_cmd = v_in;

  // 안전 집합 𝒮 = { safe 인 class }.
  std::vector<ClassEval> S;
  for (const auto & e : evals) if (e.safe) S.push_back(e);

  // c* 가 안전 집합에 있는지.
  bool cstar_in_S = false;
  for (const auto & e : S) {
    if (e.cls.equals(st.c_star)) { cstar_in_S = true; break; }
  }

  // ---- 2) [안전] 사전식 최상위 분기 (P3 안전 지배) -----------------------
  if (!cstar_in_S) {
    if (trace) trace->safety_branch = true;
    SafetyResult sr = run_safety_branch(st, S, v_cmd, ttc, now, k_, tb_);
    out.c_star = st.c_star;
    out.v_target = sr.v_target;
    out.safety_velocity_limited = sr.velocity_limited;
    out.mode = st.mode;
    // HOLD 진입 시: 새 최선 class 로 커밋 + e_fwd,e_rev,ρ,드웰 리셋.
    if (st.mode == Mode::HOLD && !S.empty()) {
      // 새 최선 안전 class.
      const ClassEval * best = nullptr;
      for (const auto & e : S) {
        if (best == nullptr || e.J < best->J) best = &e;
      }
      if (best != nullptr) {
        st.c_star = best->cls;
        st.e_fwd = 0; st.e_rev = 0; st.rho = 0;
        st.t_safe_dwell = now + k_.T_safe_dwell;
        out.c_star = st.c_star;
      }
    }
    finalize_trace(out);
    return out;   // FINALIZE (안전 분기는 ρ 갱신 생략)
  }

  // ---- 3) [재량] 분리 누적기 커밋 전환 규칙 (P1·P2) ----------------------
  std::vector<ClassEval> challengers;
  for (const auto & e : S) if (!e.cls.equals(st.c_star)) challengers.push_back(e);

  if (!challengers.empty()) {
    TopoClass c_prime = tb_.pick(challengers, st, k_);
    // J(c*), J(c′).
    double J_cstar = std::numeric_limits<double>::infinity();
    double J_cprime = std::numeric_limits<double>::infinity();
    for (const auto & e : evals) {
      if (e.cls.equals(st.c_star)) J_cstar = e.J;
      if (e.cls.equals(c_prime))   J_cprime = e.J;
    }
    const double D = J_cstar - J_cprime;   // 도전자 우위

    if (trace) { trace->challenger = c_prime; trace->advantage = D; }
    // 도전 대상 변경 시 e_rev 리셋.
    if (!st.prev_c_prime.has_value() || !c_prime.equals(st.prev_c_prime.value())) {
      st.e_rev = 0;
      if (trace) trace->challenger_reset = true;
    }
    if (trace) trace->evidence_after_challenger_reset = st.e_rev;

    // 분리 누적기 갱신 (e_rev: (D-Δ)dt, e_fwd: (-D-Δ)dt).
    accumulate(st, D, dt, k_);

    if (trace) {
      trace->evidence_after_accumulate = st.e_rev;
      trace->rho_used = st.rho;
      trace->threshold_used = E_th(st.rho, k_);
    }
    // 전환 조건: e_rev >= E_th(ρ).
    if (switch_ready(st, k_)) {
      st.c_star = c_prime;
      discretionary_commit = true;
      if (trace) trace->commit_reset = true;
      st.reset_on_commit();   // e_rev, ρ, L_real, e_fwd 리셋
    }
    st.prev_c_prime = c_prime;
  }

  // ---- 4) ρ 갱신 · 출력 -------------------------------------------------
  // ΔL_k = v_lat·dt − b_k 근사: 전진 속도 기반 cross-track 진행 (단순 양수 진행).
  const double v_lat = std::abs(v_cmd);
  // Measured delta belongs to the pre-step commitment: discard it on a commit.
  // Retreat decreases progress, bounded below by zero. Legacy calls stay identical.
  if (progress_delta_m) {
    if (!discretionary_commit)
      st.L_real = std::max(0.0, st.L_real + *progress_delta_m);
  } else {
    st.L_real += v_lat * dt;
  }
  if (st.L_plan > 0.0) {
    st.rho = std::max(0.0, std::min(1.0, st.L_real / st.L_plan));
  }

  out.c_star = st.c_star;
  out.v_target = v_cmd;
  out.mode = st.mode;
  finalize_trace(out);
  return out;
}

}  // namespace compass
