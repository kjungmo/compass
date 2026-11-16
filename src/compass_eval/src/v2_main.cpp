// v2_main.cpp — experiments added in v2 after review (post hoc). Plan:
// paper/arxiv_ref/V2_EXPERIMENT_PLAN.md (committed before this ran).
//   compass_eval_v2 all <outdir>
// writes tuning_grid.csv, selected_params.csv, v2_native_raw.csv,
// v2_sigma_raw.csv, v2_late_reversal_raw.csv.
#include "compass_eval/harness_v2.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

using namespace compass_eval;
using namespace compass;

static const int EVAL_SEEDS = 50;
static const uint32_t TUNE_BASE = 1000;

static std::string fmt(double v) { std::ostringstream o; o << v; return o.str(); }

static Policy core_policy(const std::string & id, const std::string & label, Knobs k) {
  Policy p; p.id = id; p.label = label; p.kind = PKind::Core; p.knobs = k; return p;
}
static Policy dwell_policy(const std::string & id, double t) {
  Policy p; p.id = id; p.label = "T_dwell=" + fmt(t); p.kind = PKind::Dwell; p.p1 = t; return p;
}
static Policy cw_policy(const std::string & id, double b) {
  Policy p; p.id = id; p.label = "b=" + fmt(b); p.kind = PKind::CW; p.p1 = b; return p;
}
static Policy li_policy(const std::string & id, double tmin, double r) {
  Policy p; p.id = id; p.label = "T_min=" + fmt(tmin) + ",r=" + fmt(r);
  p.kind = PKind::LI; p.p1 = tmin; p.p2 = r; return p;
}
static Policy e0_policy(const std::string & id, double e0) {
  Knobs k; k.E0 = e0; return core_policy(id, "E0=" + fmt(e0), k);
}

struct Grid { std::string rule; std::vector<Policy> pts; };

static std::vector<Grid> make_grids() {
  std::vector<Grid> g;
  Grid a; a.rule = "compass_E0";
  for (double e : {0.10, 0.15, 0.20, 0.30, 0.40}) a.pts.push_back(e0_policy("compass_E0", e));
  g.push_back(a);
  Grid d; d.rule = "dwell";
  for (double t : {0.2, 0.4, 0.6, 1.0, 1.5, 2.0}) d.pts.push_back(dwell_policy("dwell", t));
  g.push_back(d);
  Grid c; c.rule = "cw";
  for (double b : {0.00, 0.05, 0.10, 0.15, 0.20, 0.30, 0.40}) c.pts.push_back(cw_policy("cw", b));
  g.push_back(c);
  Grid l; l.rule = "li";
  for (double t : {0.3, 0.6, 1.0, 2.0}) for (double r : {0.05, 0.10, 0.20, 0.40})
    l.pts.push_back(li_policy("li", t, r));
  g.push_back(l);
  return g;
}

int main(int argc, char ** argv) {
  if (argc < 3 || std::string(argv[1]) != "all") {
    fprintf(stderr, "usage: compass_eval_v2 all <outdir>\n"); return 2;
  }
  const std::string out = argv[2];
  const auto scns = all_scns();
  auto grids = make_grids();

  // ---------------- tuning grid: tuning seeds (loss) and evaluation seeds (reported)
  std::ofstream tg(out + "/tuning_grid.csv");
  tg << "split,rule,param,scenario,n,mean_switches,mean_excess_switches,final_side_hits,"
        "final_side_n,mean_loss\n";
  std::ofstream sp(out + "/selected_params.csv");
  sp << "rule,param,tuning_mean_loss\n";
  std::vector<Policy> selected;   // order: compass_E0, dwell, cw, li
  for (const auto & g : grids) {
    double best = 1e18; size_t best_i = 0;
    for (size_t pi = 0; pi < g.pts.size(); ++pi) {
      const Policy & p = g.pts[pi];
      double total = 0; int cnt = 0;
      for (int split = 0; split < 2; ++split) {
        for (auto s : scns) {
          const Target tg_ = target_of(s);
          double sw = 0, ex = 0, ls = 0; int hits = 0, hn = 0;
          for (int k = 0; k < EVAL_SEEDS; ++k) {
            const uint32_t seed = (split == 0 ? TUNE_BASE : 0) + k;
            auto sc = make_scenario_v2(s, seed);
            Run r = score(run_policy(p, sc, seed));
            const int excess = std::max(0, r.m.switches - tg_.min_switches);
            const int wrong = (tg_.side >= 0 && r.final_side != tg_.side) ? 1 : 0;
            sw += r.m.switches; ex += excess; ls += excess + 3 * wrong;
            if (tg_.side >= 0) { ++hn; if (!wrong) ++hits; }
            if (split == 0) { total += excess + 3 * wrong; ++cnt; }
          }
          tg << (split == 0 ? "tune" : "eval") << ',' << g.rule << ",\"" << p.label << "\","
             << scn_name(s) << ',' << EVAL_SEEDS << ',' << sw / EVAL_SEEDS << ','
             << ex / EVAL_SEEDS << ',' << hits << ',' << hn << ',' << ls / EVAL_SEEDS << '\n';
        }
      }
      const double mean_loss = total / cnt;
      if (mean_loss < best - 1e-12) { best = mean_loss; best_i = pi; }
    }
    sp << g.rule << ",\"" << g.pts[best_i].label << "\"," << best << '\n';
    Policy s = g.pts[best_i];
    s.id = g.rule == "compass_E0" ? "compass_E0_tuned" : g.rule + "_tuned";
    selected.push_back(s);
  }
  tg.close(); sp.close();
  const Policy & sel_e0 = selected[0];
  const Policy & sel_dw = selected[1];
  const Policy & sel_cw = selected[2];
  const Policy & sel_li = selected[3];

  // ---------------- policy sets
  Knobs kfull; Knobs khyst; khyst.delta_floor = 0; Knobs khard; khard.k_rho = 0;
  Knobs klam; klam.lambda = 1.0;
  Policy full = core_policy("full", "default", kfull);
  Policy hyst = core_policy("hyst0", "delta_floor=0", khyst);
  Policy hard = core_policy("hard0", "k_rho=0", khard);
  Policy lam1 = core_policy("lambda1", "lambda=1", klam);
  Policy argmin; argmin.id = "argmin"; argmin.label = "immediate"; argmin.kind = PKind::Argmin;
  Policy dwell06 = dwell_policy("dwell_0.6", 0.6);
  Policy noclass; noclass.id = "noclass"; noclass.label = "band=0.10"; noclass.kind = PKind::NoClass;

  // ---------------- native sigma, evaluation seeds
  {
    std::ofstream f(out + "/v2_native_raw.csv");
    f << "policy,param,scenario,seed,switches,sign_change_rate,decision_entropy,t_sfx_s,"
         "censored,final_side,target_side,hit\n";
    std::vector<Policy> pols = {full, hyst, hard, argmin, dwell06, noclass, lam1,
                                sel_e0, sel_dw, sel_cw, sel_li};
    for (const auto & p : pols) for (auto s : scns) for (int seed = 0; seed < EVAL_SEEDS; ++seed) {
      Run r = score(run_policy(p, make_scenario_v2(s, seed), seed));
      const Target t = target_of(s);
      f << p.id << ",\"" << p.label << "\"," << scn_name(s) << ',' << seed << ','
        << r.m.switches << ',' << r.m.sign_change_rate << ',' << r.m.entropy << ','
        << r.m.t_legible << ',' << (r.m.censored ? 1 : 0) << ',' << r.final_side << ','
        << t.side << ',' << (t.side < 0 ? -1 : (r.final_side == t.side ? 1 : 0)) << '\n';
    }
  }

  // ---------------- uniform sigma sweep
  {
    std::ofstream f(out + "/v2_sigma_raw.csv");
    f << "policy,param,sigma,scenario,seed,switches,t_sfx_s,censored,final_side,target_side,hit\n";
    std::vector<Policy> pols = {full, hyst, hard, lam1, sel_e0, argmin, dwell06, sel_dw, sel_cw, sel_li};
    for (double sg : {0.03, 0.06, 0.12, 0.20, 0.30})
      for (const auto & p : pols) for (auto s : scns) for (int seed = 0; seed < EVAL_SEEDS; ++seed) {
        Run r = score(run_policy(p, make_scenario_v2(s, seed, 0.05, sg), seed));
        const Target t = target_of(s);
        f << p.id << ",\"" << p.label << "\"," << sg << ',' << scn_name(s) << ',' << seed << ','
          << r.m.switches << ',' << r.m.t_legible << ',' << (r.m.censored ? 1 : 0) << ','
          << r.final_side << ',' << t.side << ','
          << (t.side < 0 ? -1 : (r.final_side == t.side ? 1 : 0)) << '\n';
      }
  }

  // ---------------- late spurious reversal
  {
    std::ofstream f(out + "/v2_late_reversal_raw.csv");
    f << "policy,param,n_burst,seed,switches,final_side,hit,reversed_in_burst,rho_at_burst_start\n";
    std::vector<Policy> pols = {full, hard, hyst, lam1, argmin, dwell06, sel_dw, sel_cw, sel_li};
    for (int nb : {20, 30, 40, 60})
      for (const auto & p : pols) for (int seed = 0; seed < EVAL_SEEDS; ++seed) {
        double rho = -1.0;
        auto sc = make_late_reversal(seed, nb);
        auto sides = run_policy(p, sc, seed, LATE_BURST_START - 1, &rho);
        Run r = score(sides);
        const bool before = sides[LATE_BURST_START - 1] == 0;
        bool rev = false;
        for (int i = LATE_BURST_START; i < T; ++i) if (sides[i] == 1) rev = true;
        f << p.id << ",\"" << p.label << "\"," << nb << ',' << seed << ',' << r.m.switches << ','
          << r.final_side << ',' << (r.final_side == 0 ? 1 : 0) << ',' << ((before && rev) ? 1 : 0)
          << ',' << rho << '\n';
      }
  }
  return 0;
}
