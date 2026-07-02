// cost_evaluator.cpp
#include "compass_core/cost_evaluator.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace compass {

CostEvaluator::CostEvaluator(Knobs k) : k_(k) {}

double CostEvaluator::clip01(double x) {
  return std::max(0.0, std::min(1.0, x));
}

double CostEvaluator::social_kernel(const Point2D & query, const Person & p) const {
  // δ = query − p̂.
  const double dx = query.x - p.pose.x;
  const double dy = query.y - p.pose.y;

  // 사람-헤딩 국소 좌표로 사영하여 전/후방 반평면 귀속 (단일 부호 판정).
  const double ct = std::cos(p.pose.theta);
  const double st = std::sin(p.pose.theta);
  const double d_long = dx * ct + dy * st;    // 헤딩 방향 성분
  const double sigma_h = (d_long >= 0.0) ? k_.sigma_front : k_.sigma_rear;
  const double sigma_s = k_.sigma_s;

  // M = R·diag(σ_h², σ_s²)·Rᵀ + Σ.  (R 은 헤딩 회전.)
  const double a = sigma_h * sigma_h;   // 종(헤딩축) 분산
  const double b = sigma_s * sigma_s;   // 횡 분산
  // R·diag(a,b)·Rᵀ 의 성분:
  double m00 = a * ct * ct + b * st * st;
  double m01 = (a - b) * ct * st;
  double m11 = a * st * st + b * ct * ct;
  // + 예측 공분산 Σ (행 우선 [c00,c01,c10,c11]).
  m00 += p.cov[0];
  m01 += 0.5 * (p.cov[1] + p.cov[2]);
  m11 += p.cov[3];

  // δᵀ M⁻¹ δ.
  const double det = m00 * m11 - m01 * m01;
  if (det <= 0.0) return 0.0;            // 퇴화 -> 침해 없음으로 간주
  const double inv00 =  m11 / det;
  const double inv01 = -m01 / det;
  const double inv11 =  m00 / det;
  const double q = dx * (inv00 * dx + inv01 * dy) + dy * (inv01 * dx + inv11 * dy);
  return std::exp(-0.5 * q);
}

double CostEvaluator::j_goal_raw(const TopoClass &, const DecisionInput & in) const {
  // 지평선 끝 로봇 위치 근사: 현재 속도로 짧은 지평선 전진 (등속).
  const double T = 1.0;   // 평가 지평선 (s) — 고정 상수
  const double px = in.robot_pose.x + in.robot_vel.vx * std::cos(in.robot_pose.theta) * T;
  const double py = in.robot_pose.y + in.robot_vel.vx * std::sin(in.robot_pose.theta) * T;
  const double dx = px - in.local_goal.x;
  const double dy = py - in.local_goal.y;
  return std::sqrt(dx * dx + dy * dy);
}

double CostEvaluator::j_social_raw(const TopoClass &, const DecisionInput & in) const {
  // J_social = Σ_i ∫₀ᵀ g_i(p_r(t)) dt — 로봇 직선 등속 롤아웃을 따라 사다리꼴 적분.
  const double T = 1.0;
  const int N = 10;
  const double dt = T / N;
  const double ct = std::cos(in.robot_pose.theta);
  const double st = std::sin(in.robot_pose.theta);
  double total = 0.0;
  for (const auto & p : in.people) {
    double acc = 0.0;
    for (int n = 0; n <= N; ++n) {
      const double tt = n * dt;
      Point2D pr;
      pr.x = in.robot_pose.x + in.robot_vel.vx * ct * tt;
      pr.y = in.robot_pose.y + in.robot_vel.vx * st * tt;
      const double w = (n == 0 || n == N) ? 0.5 : 1.0;  // 사다리꼴 가중
      acc += w * social_kernel(pr, p);
    }
    total += acc * dt;
  }
  return total;
}

double CostEvaluator::j_effort_raw(const TopoClass &, const DecisionInput & in) const {
  // J_effort 근사: 각속(곡률 대용) 제곱에 비례 — 결정적·시불변.
  const double w = in.robot_vel.wz;
  return w * w;
}

double CostEvaluator::j_rule_raw(const TopoClass & c, const DecisionInput &) const {
  // J_rule: locale.keep_side 소프트 벌점 — 기본은 우측 통행 선호.
  // 모든 관련자에 대해 L 통과는 소프트 벌점(우측 통행 규약 위반)으로 가산.
  double penalty = 0.0;
  for (const auto & [id, side] : c.pairs()) {
    (void)id;
    if (side == Side::L) penalty += 1.0;
  }
  return c.size() > 0 ? penalty / static_cast<double>(c.size()) : 0.0;
}

double CostEvaluator::J(const TopoClass & c, const DecisionInput & in,
                        const IEnvQuery & env) const {
  // 하드 가용성 (별도, J=+∞): 통로 폭 부족 / clearance·d_safe 위반 / 비실현가능.
  const double robot_width = 0.5;        // 원형 풋프린트 근사 폭 (m)
  const double safety_margin = 0.1;
  if (env.corridor_width(c) < robot_width + safety_margin) {
    return std::numeric_limits<double>::infinity();
  }
  if (env.clearance(c) < k_.d_safe) {
    return std::numeric_limits<double>::infinity();
  }
  if (!env.feasible(c)) {
    return std::numeric_limits<double>::infinity();
  }

  // 항별 정규화 (고정 보정 상수 -> 시불변).
  const double jg = clip01((j_goal_raw(c, in)   - goal_min_)   / (goal_max_   - goal_min_));
  const double js = clip01((j_social_raw(c, in) - social_min_) / (social_max_ - social_min_));
  const double je = clip01((j_effort_raw(c, in) - effort_min_) / (effort_max_ - effort_min_));
  const double jr = clip01((j_rule_raw(c, in)   - rule_min_)   / (rule_max_   - rule_min_));

  return k_.w_g * jg + k_.w_s * js + k_.w_e * je + k_.w_r * jr;
}

}  // namespace compass
