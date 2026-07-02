#include <gtest/gtest.h>
#include "compass_core/decision_core.hpp"
#include "compass_core/accumulator.hpp"
#include <cmath>
using namespace compass;

// per-class 평가를 직접 만들어주는 헬퍼 (safe=true 기본).
static ClassEval mk(std::initializer_list<std::pair<uint64_t, Side>> v, double J,
                    bool safe = true) {
  ClassEval e;
  for (auto & p : v) e.cls.set(p.first, p.second);
  e.J = J; e.available = safe; e.safe = safe;
  return e;
}

// 결정적 환경 스텁: 모든 class 안전·실현가능.
struct FakeEnv : IEnvQuery {
  double corridor_width(const TopoClass &) const override { return 2.0; }
  double clearance(const TopoClass &) const override { return 1.0; }
  double ttc(const TopoClass &) const override { return 5.0; }
  bool feasible(const TopoClass &) const override { return true; }
};

// research_spec §5.3 #3: 유계 D_max 에서 연속 재량 전환 간격 ≥ E_th(ρ)/(D_max−Δ_floor).
TEST(DecisionCore, P2MinSwitchInterval) {
  Knobs k; k.lambda = 1.0;                 // 순수 적분기 (비공허 보장)
  DecisionCore core(k);
  DecisionState s; s.c_star.set(7, Side::R);

  const double dt = 0.1;
  const double D = 0.20;                    // 도전자 우위 (> Δ_floor=0.05)
  const double J_cstar = 0.50;
  const double J_cprime = J_cstar - D;      // 7L 이 더 좋음

  // v_in=0 으로 ρ=0 고정 -> E_th(0)=E0.
  int cycles = 0;
  for (int i = 0; i < 1000; ++i) {
    std::vector<ClassEval> evals = {
      mk({{7, Side::R}}, J_cstar),
      mk({{7, Side::L}}, J_cprime)
    };
    DecisionOutput o = core.step_evals(evals, s, /*ttc=*/5.0, /*v_in=*/0.0,
                                       /*now=*/i * dt, dt);
    cycles++;
    if (o.c_star.side(7).value() == Side::L) break;   // 전환 발생
  }
  // P2 하한: cycles ≥ E_th(0) / ((D − Δ_floor)·dt).
  const double lower = E_th(0.0, k) / ((D - k.delta_floor) * dt);
  EXPECT_GE(cycles, static_cast<int>(std::floor(lower)));
  EXPECT_LE(cycles, static_cast<int>(std::ceil(lower)) + 1);  // 합리적 상한
}

// research_spec §5.3 #4: ρ↑ 시 E_th(ρ) 증가로 전환 간격 하한이 ρ=0 대비 커짐.
TEST(DecisionCore, P2HardeningRaisesInterval) {
  Knobs k; k.lambda = 1.0;
  const double dt = 0.1;
  const double D = 0.20;
  const double J_cstar = 0.50, J_cprime = J_cstar - D;

  auto count = [&](double v_in, double L_plan) {
    DecisionCore core(k);
    DecisionState s; s.c_star.set(7, Side::R); s.L_plan = L_plan;
    int cycles = 0;
    for (int i = 0; i < 5000; ++i) {
      std::vector<ClassEval> evals = {
        mk({{7, Side::R}}, J_cstar), mk({{7, Side::L}}, J_cprime)
      };
      DecisionOutput o = core.step_evals(evals, s, 5.0, v_in, i * dt, dt);
      cycles++;
      if (o.c_star.side(7).value() == Side::L) break;
    }
    return cycles;
  };

  // ρ=0 (v_in=0): E_th=E0.  ρ→1 (v_in>0, 작은 L_plan): E_th=E0(1+k_rho).
  int cyc_rho0    = count(/*v_in=*/0.0, /*L_plan=*/1.0);
  int cyc_rhohigh = count(/*v_in=*/1.0, /*L_plan=*/0.001);  // ρ 한 주기에 1로 포화
  EXPECT_GT(cyc_rhohigh, cyc_rho0);   // 진행 경화로 간격 증가
}

// research_spec §5.3 #11: 봉쇄 조건 + ρ→1 유한 완료 (P4).
TEST(DecisionCore, P4BlockingAndCompletion) {
  Knobs k; k.lambda = 1.0;             // e^rev_max = e_max_rev = 0.5
  // 봉쇄 존재 조건: E_0 < e^rev_max ≤ E_0(1+k_rho) => 0.3 < 0.5 ≤ 0.6  ✓
  EXPECT_GT(k.e_max_rev, k.E0);
  EXPECT_LE(k.e_max_rev, k.E0 * (1 + k.k_rho));
  // ρ̄: e_rev_max 에서 봉쇄 시작하는 진행도. λ=1 이면 reach = e_max_rev.
  double rb = rho_bar(/*D_max=*/1.0, /*dt=*/0.1, k);
  EXPECT_GT(rb, 0.0); EXPECT_LE(rb, 1.0);

  DecisionCore core(k);
  DecisionState s; s.c_star.set(7, Side::R); s.L_plan = 1.0;
  const double dt = 0.1;
  const double D = 0.20, J_cstar = 0.50, J_cprime = J_cstar - D;

  // ρ 가 ρ̄ 이상이고 도전 누적이 상한(e_max_rev<E_th(ρ))이면 전환 봉쇄.
  // 먼저 ρ 를 1 근처로 끌어올려 E_th(ρ) > e_max_rev 가 되도록.
  bool switched = false;
  int finite_cycles = 0;
  for (int i = 0; i < 2000; ++i) {
    std::vector<ClassEval> evals = {
      mk({{7, Side::R}}, J_cstar), mk({{7, Side::L}}, J_cprime)
    };
    DecisionOutput o = core.step_evals(evals, s, 5.0, /*v_in=*/1.0, i * dt, dt);
    finite_cycles++;
    if (o.c_star.side(7).value() == Side::L) { switched = true; break; }
    if (s.rho >= 1.0) {
      // ρ=1 에서 E_th(1)=E0(1+k_rho)=0.6 > e_max_rev=0.5 -> 봉쇄.
      // 추가 주기를 더 돌려도 e_rev 가 상한 0.5 에 묶여 전환 불가.
      for (int j = 0; j < 200; ++j) {
        DecisionOutput o2 = core.step_evals(evals, s, 5.0, 1.0, (i + j) * dt, dt);
        ASSERT_FALSE(o2.c_star.side(7).value() == Side::L);  // 봉쇄 유지
      }
      break;
    }
  }
  // ρ 가 유한 주기 내에 1 에 도달 (v_lat>0 으로 완료) — 무한 루프 없음.
  EXPECT_LE(finite_cycles, 2000);
  EXPECT_DOUBLE_EQ(s.rho, 1.0);
  EXPECT_FALSE(switched);   // 봉쇄로 전환되지 않음
}

// research_spec §5.3 #12: 리셋 의미 (c′≠prev_c′ -> e_rev=0; 재량 전환 리셋; 안전 전환 리셋).
TEST(DecisionCore, ResetSemantics) {
  Knobs k; k.lambda = 1.0;
  const double dt = 0.1;

  // (a) c′ ≠ prev_c′ -> e_rev 리셋.
  {
    DecisionCore core(k);
    DecisionState s; s.c_star.set(7, Side::R); s.L_plan = 1.0;
    // 첫 주기: 도전자 7L.
    std::vector<ClassEval> e1 = { mk({{7, Side::R}}, 0.50), mk({{7, Side::L}}, 0.40) };
    core.step_evals(e1, s, 5.0, 0.0, 0.0, dt);
    double e_rev_after1 = s.e_rev;
    EXPECT_GT(e_rev_after1, 0.0);   // 누적됨
    // 둘째 주기: 도전자가 9L 로 바뀜 -> e_rev 리셋 후 새로 누적 (1주기치만).
    std::vector<ClassEval> e2 = { mk({{7, Side::R}}, 0.50), mk({{9, Side::L}}, 0.40) };
    core.step_evals(e2, s, 5.0, 0.0, dt, dt);
    // 리셋되었으므로 두 주기 누적이 아니라 한 주기치와 같아야 함.
    DecisionCore core_ref(k);
    DecisionState sref; sref.c_star.set(7, Side::R); sref.L_plan = 1.0;
    core_ref.step_evals(e2, sref, 5.0, 0.0, 0.0, dt);
    EXPECT_NEAR(s.e_rev, sref.e_rev, 1e-12);
  }

  // (b) 재량 전환 -> e_rev=0, ρ=0, L_real=0, e_fwd=0.
  {
    DecisionCore core(k);
    DecisionState s; s.c_star.set(7, Side::R); s.L_plan = 1.0;
    std::vector<ClassEval> ev = { mk({{7, Side::R}}, 0.50), mk({{7, Side::L}}, 0.30) };
    for (int i = 0; i < 1000; ++i) {
      DecisionOutput o = core.step_evals(ev, s, 5.0, 0.5, i * dt, dt);
      if (o.c_star.side(7).value() == Side::L) {
        EXPECT_DOUBLE_EQ(s.e_rev, 0.0);
        EXPECT_DOUBLE_EQ(s.rho, 0.0);
        EXPECT_DOUBLE_EQ(s.L_real, 0.0);
        EXPECT_DOUBLE_EQ(s.e_fwd, 0.0);
        break;
      }
    }
  }

  // (c) 안전 전환 -> e_rev=0, ρ=0, t_safe_dwell 설정.
  {
    DecisionCore core(k);
    DecisionState s; s.c_star.set(7, Side::R); s.e_rev = 0.4; s.rho = 0.5;
    // c* = 7R 가 안전 집합 밖 (S = {7L} 만 안전).
    std::vector<ClassEval> ev = {
      mk({{7, Side::R}}, 0.50, /*safe=*/false),
      mk({{7, Side::L}}, 0.30, /*safe=*/true)
    };
    core.step_evals(ev, s, /*ttc=*/5.0, /*v_in=*/0.5, /*now=*/1.0, dt);
    EXPECT_DOUBLE_EQ(s.e_rev, 0.0);
    EXPECT_DOUBLE_EQ(s.rho, 0.0);
    EXPECT_GT(s.t_safe_dwell, 1.0);   // 드웰 개시
  }
}

// 통합 스모크: 전체 step() 파이프라인 (열거+비용+IEnvQuery) 이 출력 반환.
TEST(DecisionCore, FullStepSmoke) {
  Knobs k; DecisionCore core(k); FakeEnv env;
  DecisionInput in;
  in.robot_pose = {0.0, 0.0, 0.0};
  in.robot_vel = {0.5, 0.0};
  in.local_goal = {5.0, 0.0};
  in.dt = 0.1; in.now = 0.0;
  Person p; p.id = 7; p.pose = {2.0, 0.5, 0.0};
  in.people = {p};
  DecisionState s; s.c_star.set(7, Side::R);
  DecisionOutput o = core.step(in, s, env);
  EXPECT_GE(o.v_target, 0.0);
  EXPECT_EQ(o.c_star.size(), 1u);
}

// 회귀(행동): 기본 노브(λ=0.97)에서 지속된 중간 우위(D≈0.30)는 유계 주기 내 전환을 일으키고,
// 약한 우위(D≈0.10)는 절대 전환되지 않는다 (재튜닝 전 λ=0.9 에서는 D=0.30 도 전환 불가였음).
// 정상상태 증거 e_ss=(D-Δ_floor)·dt/(1-λ): D=0.30 -> 0.4167>E0=0.30 (전환), D=0.10 -> 0.083<E0 (불가).
// v_in=0 으로 ρ=0 고정 -> E_th(0)=E0=0.30. dt=0.05 는 기본 노브 정합.
TEST(DecisionCore, DefaultKnobsSwitchOnSustainedModerateAdvantage) {
  Knobs k;  // 기본값 (λ=0.97)
  const double dt = 0.05;
  const double J_cstar = 0.50;

  auto switch_cycle = [&](double D) -> int {
    DecisionCore core(k);
    DecisionState s; s.c_star.set(7, Side::R);
    const double J_cprime = J_cstar - D;            // 7L 이 D 만큼 더 좋음
    for (int i = 0; i < 1000; ++i) {
      std::vector<ClassEval> evals = {
        mk({{7, Side::R}}, J_cstar), mk({{7, Side::L}}, J_cprime)
      };
      DecisionOutput o = core.step_evals(evals, s, /*ttc=*/5.0, /*v_in=*/0.0,
                                         /*now=*/i * dt, dt);
      if (o.c_star.side(7).value() == Side::L) return i + 1;  // 전환 발생
    }
    return -1;  // 전환 안 됨
  };

  // 중간 우위: 유계 주기(넉넉히 200) 내 전환.
  int cyc = switch_cycle(/*D=*/0.30);
  EXPECT_GT(cyc, 0);
  EXPECT_LE(cyc, 200);

  // 약한 우위: 1000 주기 내 절대 전환 없음 (e_ss<E0).
  EXPECT_EQ(switch_cycle(/*D=*/0.10), -1);
}
