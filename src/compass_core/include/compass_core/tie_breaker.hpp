// tie_breaker.hpp
#pragma once
#include <optional>
#include <vector>
#include "compass_core/decision_state.hpp"
#include "compass_core/knobs.hpp"
#include "compass_core/types.hpp"
namespace compass {

// 동률 도전자 단일화 (research_spec §4.3 R6).
//
// 규칙:
//   1) 최선 J 로부터 ε_out 이내의 도전자를 동률 집합으로 수집.
//   2) 동률 집합 내에서 prev_c_prime 우선(존재 + ε_out 이내) ->
//      c* 와의 해밍 거리 작은 쪽 -> lex_less 최소.
//   3) 경계 채터링 봉쇄: 한 번 고정된 우승자는 (현재 우승자 J) - (새 최선 J)
//      가 ε_out 을 넘어서야 풀림 (ε_in < ε_out 히스테리시스).
//
// 내부 hysteresis 상태(locked_)는 인스턴스에 보존되므로, 시간 일관성을
// 검증하려면 동일 TieBreaker 인스턴스를 주기 간 재사용해야 한다.
class TieBreaker {
public:
  TopoClass pick(const std::vector<ClassEval> & challengers,
                 const DecisionState & st, const Knobs & k);
  void reset() { locked_.reset(); }

private:
  std::optional<TopoClass> locked_;  // 현재 고정된 동률 우승자
};

}  // namespace compass
