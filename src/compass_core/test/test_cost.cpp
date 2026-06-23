#include <gtest/gtest.h>
#include "compass_core/cost_evaluator.hpp"
#include "compass_core/env_query.hpp"
#include <cmath>
using namespace compass;

// 결정적 스텁 환경: per-class 고정값 반환.
struct FakeEnv : IEnvQuery {
  double cw = 2.0, clr = 1.0, t = 5.0; bool feas = true;
  double corridor_width(const TopoClass &) const override { return cw; }
  double clearance(const TopoClass &) const override { return clr; }
  double ttc(const TopoClass &) const override { return t; }
  bool feasible(const TopoClass &) const override { return feas; }
};

static DecisionInput base_input() {
  DecisionInput in;
  in.robot_pose = {0.0, 0.0, 0.0};
  in.robot_vel = {1.0, 0.0};
  in.local_goal = {5.0, 0.0};
  in.dt = 0.1; in.now = 0.0;
  return in;
}

// research_spec §5.3 #14: 사전 보정 상수 정규화에서 무차원 우위 의미가 주기 간 일정 (A5).
// 같은 기하 입력을 서로 다른 절대 시각(now)·동일 상대 배치로 두 번 평가하면
// 정규화 비용 J 가 동일해야 한다 (시불변 사상).
TEST(Cost, NormalizationTimeInvariance) {
  Knobs k; CostEvaluator ce(k); FakeEnv env;
  TopoClass c; c.set(7, Side::R);

  DecisionInput a = base_input(); a.now = 0.0;
  DecisionInput b = base_input(); b.now = 1000.0;   // 절대 시각만 다름
  // 동일한 사람 배치 (상대 기하 동일).
  Person p; p.id = 7; p.pose = {2.0, 0.5, 0.0}; p.vel = {0.0, 0.0};
  p.cov[0] = 0.04; p.cov[3] = 0.04;
  a.people = {p}; b.people = {p};

  double Ja = ce.J(c, a, env);
  double Jb = ce.J(c, b, env);
  EXPECT_NEAR(Ja, Jb, 1e-12);
  // 정규화 비용은 유한·비음수.
  EXPECT_GE(Ja, 0.0);
  EXPECT_TRUE(std::isfinite(Ja));
}

// proxemics 비대칭: σ_front > σ_rear 이므로 사람 전방을 같은 거리로 지나는 것이
// 후방을 지나는 것보다 비용이 크다 (research_spec §3.2 비대칭 선택).
TEST(Cost, SocialFrontCostsMoreThanRear) {
  Knobs k; CostEvaluator ce(k);
  // 사람이 +x 를 향해 헤딩 (theta=0), 원점에 위치.
  Person p; p.id = 1; p.pose = {0.0, 0.0, 0.0}; p.vel = {0.0, 0.0};
  p.cov[0] = 0.0; p.cov[1] = 0.0; p.cov[2] = 0.0; p.cov[3] = 0.0;

  Point2D front = {1.0, 0.0};   // 사람 전방 1 m
  Point2D rear  = {-1.0, 0.0};  // 사람 후방 1 m

  double g_front = ce.social_kernel(front, p);
  double g_rear  = ce.social_kernel(rear, p);
  // 전방 폭이 더 넓으므로 같은 거리에서 커널 값(개인공간 침해)이 더 크다.
  EXPECT_GT(g_front, g_rear);
}

// 헤드룸 가용성 결합: 통로 폭 부족 -> J=+∞ (하드 가용성).
TEST(Cost, NarrowCorridorIsUnavailable) {
  Knobs k; CostEvaluator ce(k); FakeEnv env; env.cw = 0.1;  // 좁은 통로
  TopoClass c; c.set(7, Side::R);
  DecisionInput in = base_input();
  double J = ce.J(c, in, env);
  EXPECT_TRUE(std::isinf(J));
}
