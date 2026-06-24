// main.cpp — COMPASS 실험 하니스 드라이버.
//   compass_eval ablation [outdir]   -> R1 ablation 표 + R4 가독성 프록시 (CSV + md)
//   compass_eval latency             -> R3 주기당 최악 지연 (2^K 무가지치기 worst-case)
#include "compass_eval/harness.hpp"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <chrono>
#include <numeric>
#include <string>

using namespace compass_eval;
using namespace compass;

static double mean_of(const std::vector<double> & v) {
  if (v.empty()) return 0.0;
  return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}
static double std_of(const std::vector<double> & v) {
  if (v.size() < 2) return 0.0;
  const double m = mean_of(v);
  double s = 0.0; for (double x : v) s += (x - m) * (x - m);
  return std::sqrt(s / (v.size() - 1));
}

static std::string cpu_model() {
  std::ifstream f("/proc/cpuinfo");
  std::string line;
  while (std::getline(f, line)) {
    if (line.rfind("model name", 0) == 0) {
      auto pos = line.find(':');
      if (pos != std::string::npos) return line.substr(pos + 2);
    }
  }
  return "unknown";
}

static void run_ablation(const std::string & outdir) {
  const int SEEDS = 50;
  auto scns = all_scns();
  auto vars = all_vars();

  // 집계: 변형별 전 시나리오×시드 분포 + 변형×시나리오 전환수.
  std::vector<std::vector<double>> sw(vars.size()), rate(vars.size()),
      ent(vars.size()), leg(vars.size());
  std::vector<int> cens(vars.size(), 0);
  // 변형×시나리오 전환수 평균.
  std::vector<std::vector<std::vector<double>>> sw_by_scn(
      vars.size(), std::vector<std::vector<double>>(scns.size()));
  std::vector<std::vector<std::vector<double>>> leg_by_scn(
      vars.size(), std::vector<std::vector<double>>(scns.size()));
  std::vector<std::vector<int>> cens_by_scn(vars.size(), std::vector<int>(scns.size(), 0));

  std::ofstream csv(outdir + "/ablation_raw.csv");
  csv << "variant,scenario,seed,switches,sign_change_rate,decision_entropy,t_legible_s,censored\n";

  for (size_t vi = 0; vi < vars.size(); ++vi) {
    for (size_t si = 0; si < scns.size(); ++si) {
      for (int seed = 0; seed < SEEDS; ++seed) {
        auto sc = make_scenario(scns[si], static_cast<uint32_t>(seed));
        auto sides = run_variant(vars[vi], sc, static_cast<uint32_t>(seed));
        Metrics m = compute(sides);
        sw[vi].push_back(m.switches);
        rate[vi].push_back(m.sign_change_rate);
        ent[vi].push_back(m.entropy);
        leg[vi].push_back(m.t_legible);
        if (m.censored) { cens[vi]++; cens_by_scn[vi][si]++; }
        sw_by_scn[vi][si].push_back(m.switches);
        leg_by_scn[vi][si].push_back(m.t_legible);
        csv << '"' << var_name(vars[vi]) << "\"," << scn_name(scns[si]) << ',' << seed << ','
            << m.switches << ',' << m.sign_change_rate << ',' << m.entropy << ','
            << m.t_legible << ',' << (m.censored ? 1 : 0) << '\n';
      }
    }
  }
  csv.close();

  const int N = SEEDS * static_cast<int>(scns.size());
  printf("# COMPASS R1 ablation — measured (offline decision-core harness)\n");
  printf("CPU: %s\n", cpu_model().c_str());
  printf("N = %d runs/variant (%zu scenarios x %d seeds), dt=%.3f s, horizon=%d cycles (%.1f s)\n\n",
         N, scns.size(), SEEDS, DT, T, T * DT);

  printf("## 종합 (전 시나리오 평균 +/- 표준편차, N=%d)\n\n", N);
  printf("| 변형 | 전환수/조우 | 부호변화율(/s) | 결정엔트로피(bits) | time-to-legible(s) | 검열율 |\n");
  printf("|---|---|---|---|---|---|\n");
  for (size_t vi = 0; vi < vars.size(); ++vi) {
    printf("| %s | %.2f +/- %.2f | %.3f +/- %.3f | %.3f +/- %.3f | %.2f +/- %.2f | %d/%d |\n",
           var_name(vars[vi]),
           mean_of(sw[vi]), std_of(sw[vi]),
           mean_of(rate[vi]), std_of(rate[vi]),
           mean_of(ent[vi]), std_of(ent[vi]),
           mean_of(leg[vi]), std_of(leg[vi]),
           cens[vi], N);
  }

  printf("\n## 시나리오별 전환수/조우 (평균 +/- SD, N=%d/cell)\n\n", SEEDS);
  printf("| 변형 |");
  for (auto s : scns) printf(" %s |", scn_name(s));
  printf("\n|---|"); for (size_t i = 0; i < scns.size(); ++i) printf("---|"); printf("\n");
  for (size_t vi = 0; vi < vars.size(); ++vi) {
    printf("| %s |", var_name(vars[vi]));
    for (size_t si = 0; si < scns.size(); ++si)
      printf(" %.2f+/-%.2f |", mean_of(sw_by_scn[vi][si]), std_of(sw_by_scn[vi][si]));
    printf("\n");
  }

  printf("\n## 시나리오별 time-to-legible (s, 평균; 괄호=검열수/%d)\n\n", SEEDS);
  printf("| 변형 |");
  for (auto s : scns) printf(" %s |", scn_name(s));
  printf("\n|---|"); for (size_t i = 0; i < scns.size(); ++i) printf("---|"); printf("\n");
  for (size_t vi = 0; vi < vars.size(); ++vi) {
    printf("| %s |", var_name(vars[vi]));
    for (size_t si = 0; si < scns.size(); ++si)
      printf(" %.2f (%d) |", mean_of(leg_by_scn[vi][si]), cens_by_scn[vi][si]);
    printf("\n");
  }
  printf("\nraw CSV -> %s/ablation_raw.csv\n", outdir.c_str());
}

struct FakeEnv : IEnvQuery {
  double corridor_width(const TopoClass &) const override { return 2.0; }
  double clearance(const TopoClass &) const override { return 1.0; }
  double ttc(const TopoClass &) const override { return 5.0; }
  bool feasible(const TopoClass &) const override { return true; }
};

static void run_latency() {
  Knobs k; DecisionCore core(k); FakeEnv env;
  printf("# COMPASS R3 — per-cycle worst-case latency (2^K 무가지치기, 전 class 안전)\n");
  printf("CPU: %s\n", cpu_model().c_str());
  printf("측정: DecisionCore::step (열거+비용적분 J+안전판정+결정), 20000 iters/조건.\n\n");
  printf("| 관련자 K | 명시 열거 2^K | 평균(us) | p50(us) | p99(us) | 최대(us) | 20Hz 예산(50ms) 점유 |\n");
  printf("|---|---|---|---|---|---|---|\n");
  for (int K = 1; K <= 3; ++K) {
    DecisionInput in;
    in.robot_pose = {0, 0, 0}; in.robot_vel = {0.5, 0}; in.local_goal = {5, 0};
    in.dt = 0.05; in.now = 0;
    DecisionState st0;
    for (int j = 0; j < K; ++j) {
      Person p; p.id = 10 + j;
      p.pose = {2.0 + 0.3 * j, (j % 2 ? 0.6 : -0.6), 0.0};
      p.vel = {0.0, 0.3};
      in.people.push_back(p);
      st0.c_star.set(p.id, Side::R);   // c* ∈ S -> 재량 경로 구동
    }
    st0.L_plan = 1.0;
    for (int w = 0; w < 2000; ++w) { DecisionState s = st0; core.step(in, s, env); }
    std::vector<double> us; us.reserve(20000);
    for (int it = 0; it < 20000; ++it) {
      DecisionState s = st0;
      auto t0 = std::chrono::steady_clock::now();
      core.step(in, s, env);
      auto t1 = std::chrono::steady_clock::now();
      us.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
    }
    std::sort(us.begin(), us.end());
    const double mean = mean_of(us);
    const double p50 = us[us.size() / 2];
    const double p99 = us[static_cast<size_t>(us.size() * 0.99)];
    const double mx = us.back();
    printf("| %d | %d | %.3f | %.3f | %.3f | %.3f | %.4f%% |\n",
           K, 1 << K, mean, p50, p99, mx, 100.0 * mx / 50000.0);
  }
  printf("\n(p99/max << 50 ms 주기 예산 -> 20 Hz 실시간 여유 확인. |S_k| 가지치기 후 분포는 실 costmap 의존 [TBD].)\n");
}

// R5 보조: ρ-구동률(횡 진행률 v_lat) 민감도. ρ 가 빠르게 포화하면 progress
// hardening 이 E_th 를 E0(1+k_rho) 로 올려 정당한 전환마저 봉쇄(freezing)하는
// 구간을 특성화한다. clean_commit(명확한 L 우위)에서 full 의 전환·가독성을 측정.
static void run_rho_sweep() {
  printf("# COMPASS R5 보조 — ρ-구동률(v_lat) 민감도 (full, clean_commit, 50 seeds)\n");
  printf("ρ 는 횡 기동 진행도. v_lat 이 클수록 ρ 가 빨리 포화 -> E_th=E0(1+k_rho) 로\n");
  printf("상승해 명확한 우위에서도 전환이 봉쇄(freezing)됨. (코어의 v_lat=|v_cmd| 단순화 노출)\n\n");
  printf("| v_lat (m/s) | ρ_end(근사) | 전환율(전환=1 비율) | 평균 전환수 | 평균 t-legible(s) | 검열수/50 |\n");
  printf("|---|---|---|---|---|---|\n");
  const int SEEDS = 50;
  for (double vlat : {0.05, 0.10, 0.20, 0.35, 0.50}) {
    int switched = 0, cens = 0; double sw_sum = 0, leg_sum = 0;
    for (int seed = 0; seed < SEEDS; ++seed) {
      auto sc = make_scenario(Scn::CleanCommit, static_cast<uint32_t>(seed), vlat);
      auto sides = run_variant(Var::Full, sc, static_cast<uint32_t>(seed));
      Metrics m = compute(sides);
      if (m.switches >= 1) switched++;
      sw_sum += m.switches; leg_sum += m.t_legible;
      if (m.censored) cens++;
    }
    const double rho_end = std::min(1.0, vlat * DT * T / 1.0);   // L_real/L_plan, L_plan=1
    printf("| %.2f | %.2f | %d/%d | %.2f | %.2f | %d |\n",
           vlat, rho_end, switched, SEEDS, sw_sum / SEEDS, leg_sum / SEEDS, cens);
  }
  printf("\n(전환=1 비율이 1.0 -> 0.0 으로 떨어지는 v_lat 이 freezing 임계. 기본 노브의 좁은\n");
  printf("비봉쇄 구간을 보여주며, R5 의 ρ-경화·v_lat 민감도 측정 항목과 직결.)\n");
}

int main(int argc, char ** argv) {
  std::string cmd = (argc > 1) ? argv[1] : "ablation";
  if (cmd == "latency")   { run_latency();   return 0; }
  if (cmd == "rho_sweep") { run_rho_sweep(); return 0; }
  std::string outdir = (argc > 2) ? argv[2] : ".";
  run_ablation(outdir);
  return 0;
}
