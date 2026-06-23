// knobs.hpp
#pragma once
namespace compass {
// 결정 계층 노브 (research_spec §1.4). 모든 기본값은 사양서 표와 1:1.
struct Knobs {
  double delta_floor = 0.05;   // 순간 전환 마진 (무차원)
  double E0 = 0.30;            // 누적기 기준 임계 E_th(0)=E0
  double k_rho = 1.0;         // 진행 경화 계수
  int p = 2;                  // 진행 경화 지수 (p>=1)
  // λ: leak factor. Switch needs sustained (D-Delta_floor) > E_0*(1-lambda)/dt; tau_leak=-dt/ln lambda.
  double lambda = 0.97;       // 누수 계수 ∈ (0,1] (τ_leak≈1.64s @ dt=0.05)
  double e_max_fwd = 1.0;     // 정방향 누적기 상한 (O4: > E0(1+k_rho))
  double e_max_rev = 0.50;    // 역방향 누적기 상한 (P4: ∈ (E0, E0(1+k_rho)])
  double d_safe = 0.5;        // 하드 안전 거리 (m)
  double ttc_min = 2.0;       // 최소 TTC (s)
  double W = 3.0;             // 스래시 창 길이 (s)
  int n_thrash = 4;           // 스래시 임계 (회) -> N_thrash
  double w_g = 1.0;           // J_goal 가중
  double w_s = 1.0;           // J_social 가중
  double w_e = 0.3;           // J_effort 가중
  double w_r = 0.5;           // J_rule 가중
  double sigma_front = 1.2;   // 전방 proxemics 폭 (m)
  double sigma_rear = 0.6;    // 후방 proxemics 폭 (m)
  double sigma_s = 0.5;       // 측면 proxemics 폭 (m)
  int K_cap = 3;              // 명시 차원 상한
  double a_brake = 0.5;       // 감속률
  double ttc_stop = 0.8;      // 정지 임계 TTC (s)
  double T_safe_dwell = 0.6;  // 안전 드웰 보호 시간 (s)
  double eps_in = 0.02;       // 동률 진입 히스테리시스
  double eps_out = 0.06;      // 동률 이탈 히스테리시스
};
}  // namespace compass
