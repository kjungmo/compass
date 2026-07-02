// cost_evaluator.hpp
#pragma once
#include "compass_core/env_query.hpp"
#include "compass_core/knobs.hpp"
#include "compass_core/types.hpp"
namespace compass {

// 정규화 비용 J(c) 평가기 (research_spec §3.2 / §4.2).
//
//   J(c) = w_g·Ĵ_goal + w_s·Ĵ_social + w_e·Ĵ_effort + w_r·Ĵ_rule   (비가용 ⇒ +∞)
//
// 각 항은 사전 보정 상수(고정, 시불변)로 [0,1] 정규화: Ĵ_X = (J_X−min)/(max−min).
// j_social 은 문서화된 교체 가능 proxemics 함수로 유지된다.
class CostEvaluator {
public:
  explicit CostEvaluator(Knobs k);

  // 전체 정규화 비용. 하드 가용성(통로 폭·clearance·feasible) 실패 시 +∞.
  double J(const TopoClass & c, const DecisionInput & in, const IEnvQuery & env) const;

  // proxemics 커널 g_i(p) = exp(-½ δᵀ M_i⁻¹ δ), δ = p − p̂_i.
  // M_i = R_i·diag(σ_h², σ_s²)·R_iᵀ + Σ_i. σ_h 는 δ 가 사람 헤딩 전/후방
  // 반평면 어디에 귀속되는지로 σ_front/σ_rear 선택 (단일 부호 판정).
  double social_kernel(const Point2D & query, const Person & p) const;

  // 개별 항 (정규화 전 원시값) — 테스트·디버깅용.
  double j_goal_raw(const TopoClass & c, const DecisionInput & in) const;
  double j_social_raw(const TopoClass & c, const DecisionInput & in) const;
  double j_effort_raw(const TopoClass & c, const DecisionInput & in) const;
  double j_rule_raw(const TopoClass & c, const DecisionInput & in) const;

private:
  Knobs k_;
  // 고정 보정 상수 (정규화 [min,max]). 시불변 사상을 보장하는 상수.
  double goal_min_ = 0.0,   goal_max_ = 10.0;
  double social_min_ = 0.0, social_max_ = 5.0;
  double effort_min_ = 0.0, effort_max_ = 5.0;
  double rule_min_ = 0.0,   rule_max_ = 1.0;

  static double clip01(double x);
};

}  // namespace compass
