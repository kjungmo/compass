// env_query.hpp
#pragma once
#include "compass_core/types.hpp"
namespace compass {

// One-second candidate prediction. Commands apply on [t_i, t_{i+1});
// the final command is unused. Poses follow exact piecewise-constant unicycle
// integration at intervals <= .1 s, starting at the current input pose.
// Poses and people share the input world frame.
struct TrajectorySample {
  double t = 0;
  SE2 pose;
  Twist2D command;
};
using CandidateTrajectory = std::vector<TrajectorySample>;

// 환경 질의 주입 seam (research_spec §5.2). costmap·laser·운동학 의존을
// 이 인터페이스 뒤로 격리하여 core 의 순수성·단위 테스트성을 보존한다.
// compass_nav2 가 nav2_costmap_2d 위에 구현한다.
struct IEnvQuery {
  // nullopt opts into the archived evaluator. An empty/invalid supplied trajectory
  // is unavailable, never a request to silently fall back. Rebuild ABI consumers.
  virtual std::optional<CandidateTrajectory> candidate_trajectory(const TopoClass &) const {
    return std::nullopt;
  }
  // class c 의 통과 통로 폭 (m).
  virtual double corridor_width(const TopoClass & c) const = 0;
  // class c 경로의 최소 clearance (m).
  virtual double clearance(const TopoClass & c) const = 0;
  // class c 경로의 최소 TTC (s).
  virtual double ttc(const TopoClass & c) const = 0;
  // 운동학·정적 충돌 측면의 하드 실현 가능성.
  virtual bool feasible(const TopoClass & c) const = 0;
  virtual ~IEnvQuery() = default;
};

}  // namespace compass
