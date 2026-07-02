// safety.hpp
#pragma once
#include <vector>
#include "compass_core/decision_state.hpp"
#include "compass_core/knobs.hpp"
#include "compass_core/tie_breaker.hpp"
#include "compass_core/types.hpp"
namespace compass {

// 안전 사다리 분기 결과 (research_spec §3 step 2).
struct SafetyResult {
  TopoClass c_star;        // 갱신된 커밋 class (전환했으면 새 값)
  double v_target = 0.0;   // 목표 속도 (감속/정지 반영)
  Mode mode = Mode::NORMAL;
  bool took_safety_branch = false;  // 안전 분기가 재량 분기를 선점했는지
};

// 사전식 최상위 안전 분기 (research_spec §3 step 2 + §4.4 사다리).
//
// 전제: 호출자는 c* ∉ 𝒮 일 때만 본 함수를 호출한다. 본 함수는 항상
// took_safety_branch=true 를 반환하며 (P3 안전 지배), 다음 사다리를 따른다:
//   - n_thrash 증가, now 를 창 W 에 기록, W 밖 기록 제거.
//   - now < t_safe_dwell  또는 부분 임계 초과  -> 감속 (v - a_brake·dt),
//     ttc < ttc_stop 이면 STOP.
//   - 그 외 𝒮 비어있지 않으면 -> 타이브레이크로 안전 전환 + reset_on_safe_switch.
//   - 𝒮 비어있으면 -> 감속, ttc < ttc_stop 이면 STOP.
//   - n_thrash >= N_thrash -> mode = HOLD (스래시 가드, P5).
//
// 주의: 본 함수 시그니처는 dt 를 입력으로 받지 않으므로 a_brake 감속은
// knobs 의 가정 주기(=1.0초 단위 환산) 대신 a_brake 를 직접 차감한다.
SafetyResult run_safety_branch(DecisionState & s,
                               const std::vector<ClassEval> & S_set,
                               double v_in, double ttc, double now,
                               const Knobs & k, TieBreaker & tb);

}  // namespace compass
