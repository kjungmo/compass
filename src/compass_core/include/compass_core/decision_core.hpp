// decision_core.hpp
#pragma once
#include <vector>
#include "compass_core/cost_evaluator.hpp"
#include "compass_core/decision_state.hpp"
#include "compass_core/env_query.hpp"
#include "compass_core/knobs.hpp"
#include "compass_core/safety.hpp"
#include "compass_core/tie_breaker.hpp"
#include "compass_core/types.hpp"
namespace compass {

// 한 결정 주기 통합 (research_spec §3 / §4.5 알고리즘 1:1).
//
// 파이프라인: 열거 -> 비용 J -> 안전 집합 𝒮 -> [안전] 사전식 최상위 분기
// (P3 지배·P5 스래시 가드) -> [재량] 분리 누적기 커밋 전환 (P1·P2·P4) -> ρ 갱신.
struct DecisionTrace {
  double now = 0, dt = 0, v_input = 0, v_output = 0;
  DecisionState before, after;
  std::vector<ClassEval> candidates;
  std::optional<TopoClass> challenger;
  std::optional<double> advantage, evidence_after_challenger_reset,
                        evidence_after_accumulate, threshold_used, rho_used;
  bool safety_branch = false, challenger_reset = false, commit_reset = false;
  bool switched = false;
  std::optional<double> measured_progress_delta_m;
};

class DecisionCore {
public:
  explicit DecisionCore(Knobs k);

  // 전체 파이프라인: people 로부터 class 열거, CostEvaluator+IEnvQuery 로 평가.
  DecisionOutput step(const DecisionInput & in, DecisionState & st,
                      const IEnvQuery & env);

  // 결정 로직 seam: 미리 계산된 per-class 평가(evals)에 대해 결정 규칙만 실행.
  // 속성 테스트(P2/P4/리셋)가 D = J(c*)−J(c′) 를 직접 제어하기 위한 진입점.
  //   - evals: 모든 후보 class 의 ClassEval (safe 플래그 포함).
  //   - ttc, v_in: 안전 사다리 입력.
  //   - now, dt: 시각·주기.
  DecisionOutput step_evals(const std::vector<ClassEval> & evals,
                            DecisionState & st, double ttc, double v_in,
                            double now, double dt);

  // Optional caller-owned trace. Existing overloads retain their ABI.
  DecisionOutput step(const DecisionInput &, DecisionState &, const IEnvQuery &,
                      DecisionTrace *);
  DecisionOutput step_evals(const std::vector<ClassEval> &, DecisionState &,
                           double ttc, double v_in, double now, double dt,
                           DecisionTrace *);

  // Explicit signed measured progress; invalid values reject before state mutation.
  DecisionOutput step_evals(const std::vector<ClassEval> &, DecisionState &,
                           double ttc, double v_in, double now, double dt,
                           DecisionTrace *, std::optional<double> progress_delta_m);

  const Knobs & knobs() const { return k_; }

private:
  Knobs k_;
  CostEvaluator cost_;
  TieBreaker tb_;

  // class 열거: people 의 부분집합(상위 K_cap)에 대한 L/R 부호 조합 (≤ 2^K_cap).
  std::vector<TopoClass> enumerate(const DecisionInput & in) const;
};

}  // namespace compass
