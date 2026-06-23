// decision_state.hpp
#pragma once
#include <deque>
#include <optional>
#include "compass_core/topo_class.hpp"
namespace compass {
enum class Mode { NORMAL, STOP, HOLD };
// 주기 간 보존되는 결정 상태 (research_spec §1.3).
struct DecisionState {
  TopoClass c_star;                    // 현재 커밋 class c*
  double e_fwd = 0;                    // 정방향 헤드룸 누적기 e^fwd
  double e_rev = 0;                    // 역방향 도전 누적기 e^rev
  double rho = 0;                      // 진행도 ρ ∈ [0,1]
  std::optional<TopoClass> prev_c_prime;  // 직전 주기 도전자 c′
  int n_thrash = 0;                    // 안전 유발 전환 횟수
  std::deque<double> safe_switch_times;   // 창 W 내 안전 전환 시각
  double t_safe_dwell = 0;             // 안전 드웰 보호 종료 시각
  Mode mode = Mode::NORMAL;
  double L_real = 0;                   // 누적 실현 cross-track 진행량
  double L_plan = 1.0;                 // c*의 계획 횡 오프셋 (ρ 분모)

  void reset_on_commit();
  void reset_on_safe_switch(double now, double T);
};

inline void DecisionState::reset_on_commit() { e_rev = 0; rho = 0; L_real = 0; e_fwd = 0; }
inline void DecisionState::reset_on_safe_switch(double now, double T) { e_rev = 0; rho = 0; t_safe_dwell = now + T; }
}  // namespace compass
