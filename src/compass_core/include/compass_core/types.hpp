// include/compass_core/types.hpp
#pragma once
#include <cstdint>
#include <optional>
#include <vector>
#include "compass_core/topo_class.hpp"
#include "compass_core/decision_state.hpp"  // Mode

namespace compass {

// 패키지 버전 문자열 (Task 0 스모크).
const char * version();

// ---- 기하 자료형 (research_spec §2) -------------------------------------
struct Point2D { double x = 0; double y = 0; };

struct SE2 {
  double x = 0;
  double y = 0;
  double theta = 0;   // 헤딩 (rad)
};

struct Twist2D {
  double vx = 0;      // 선속 (m/s)
  double wz = 0;      // 각속 (rad/s)
};

// 추적된 사람 한 명 (research_spec §2.1).
struct Person {
  uint64_t id = 0;
  SE2 pose;
  Twist2D vel;
  // 공분산 Σ_i(t0) — 2x2 대칭, 행 우선 [c00, c01, c10, c11].
  double cov[4] = {0.0, 0.0, 0.0, 0.0};
  std::optional<bool> vulnerable_tag;
  std::optional<uint64_t> group_id;
};

// per-class 평가 결과 (research_spec §1.2).
struct ClassEval {
  TopoClass cls;
  double J = 0.0;        // 정규화 후 무차원 비용; 비가용 시 +∞
  bool available = true; // 하드 가용성 통과 여부 (false ⇒ J=∞)
  bool safe = true;      // 𝒮 멤버십 (clearance≥d_safe ∧ TTC≥TTC_min ∧ feasible)
};

// 한 결정 주기의 입력 (research_spec §2.1).
struct DecisionInput {
  SE2 robot_pose;
  Twist2D robot_vel;
  Point2D local_goal;          // g (로컬 목표)
  std::vector<Person> people;  // 추적된 사람
  double dt = 0.1;             // 주기 간격 (s)
  double now = 0.0;            // 현재 시각 (s)
};

// 한 결정 주기의 출력 (research_spec §2.2).
struct DecisionOutput {
  TopoClass c_star;   // 선택된 class
  double v_target = 0.0;  // 목표 속도 (m/s)
  Mode mode = Mode::NORMAL;
};

}  // namespace compass
