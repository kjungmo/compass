// harness_v2.hpp — experiments added in v2 after review (post hoc).
// Plan fixed before any run: paper/arxiv_ref/V2_EXPERIMENT_PLAN.md.
// Header-only, builds on harness.hpp without editing it. The archived
// scenario generator make_scenario() is untouched; make_scenario_v2() repeats
// its logic and adds an optional noise-sigma override.
#pragma once
#include <limits>
#include <string>
#include "compass_eval/harness.hpp"

namespace compass_eval {

// ---------------------------------------------------------------- scenarios
// sigma_override < 0 keeps the native per-scenario sigma (0.12 near_tie, else 0.03).
// With sigma_override < 0 the output equals make_scenario() bit for bit.
inline std::vector<Cycle> make_scenario_v2(Scn s, uint32_t seed, double vlat = 0.05,
                                           double sigma_override = -1.0) {
  std::mt19937 rng(seed);
  std::vector<Cycle> out; out.reserve(T);
  for (int i = 0; i < T; ++i) {
    double adv = 0.0, sigma = 0.03;
    switch (s) {
      case Scn::NearTie:     adv = 0.0;  sigma = 0.12; break;
      case Scn::Spike:       adv = (i >= 80 && i < 100) ? +0.25 : -0.25; break;
      case Scn::Reversal:    adv = -0.25 + 0.50 * (double(i) / (T - 1)); break;
      case Scn::CleanCommit: adv = +0.40; break;
      case Scn::Intermittent: { int ph = i % 12; adv = (ph < 8) ? +0.45 : -0.05; } break;
    }
    if (sigma_override >= 0.0) sigma = sigma_override;
    std::normal_distribution<double> nd(0.0, sigma);
    adv += nd(rng);
    Cycle c;
    c.J_R = 0.50; c.J_L = c.J_R - adv;
    c.ttc = 5.0; c.v_in = vlat; c.safe_L = true; c.safe_R = true;
    out.push_back(c);
  }
  return out;
}

// Late spurious reversal: +0.40, burst of -0.40 for nb cycles from cycle 150, then +0.40.
constexpr int LATE_BURST_START = 150;
inline std::vector<Cycle> make_late_reversal(uint32_t seed, int nb, double vlat = 0.15,
                                             double sigma = 0.03) {
  std::mt19937 rng(seed);
  std::vector<Cycle> out; out.reserve(T);
  for (int i = 0; i < T; ++i) {
    double adv = (i >= LATE_BURST_START && i < LATE_BURST_START + nb) ? -0.40 : +0.40;
    std::normal_distribution<double> nd(0.0, sigma);
    adv += nd(rng);
    Cycle c;
    c.J_R = 0.50; c.J_L = c.J_R - adv;
    c.ttc = 5.0; c.v_in = vlat; c.safe_L = true; c.safe_R = true;
    out.push_back(c);
  }
  return out;
}

// Scripted target side (plan section 0): -1 none, 0 L, 1 R; minimum switches needed.
struct Target { int side; int min_switches; };
inline Target target_of(Scn s) {
  switch (s) {
    case Scn::NearTie:      return {-1, 0};
    case Scn::Spike:        return {1, 0};
    case Scn::Reversal:     return {0, 1};
    case Scn::CleanCommit:  return {0, 1};
    default:                return {0, 1};   // intermittent
  }
}

// ---------------------------------------------------------------- new rules
// CW: previous-class consistency weight. Score = J - b*[current class]; argmin.
inline std::vector<int> run_cw(const std::vector<Cycle> & sc, double b) {
  int cur = 1; std::vector<int> out; out.reserve(sc.size());
  for (const auto & c : sc) {
    const double Jcur = cur == 1 ? c.J_R : c.J_L;
    const double Joth = cur == 1 ? c.J_L : c.J_R;
    if (c.safe_L && c.safe_R) { if (Joth < Jcur - b - 1e-9) cur = 1 - cur; }
    else if (c.safe_L) cur = 0; else if (c.safe_R) cur = 1;
    out.push_back(cur);
  }
  return out;
}

// LI: switch iff time since last switch >= t_min and (Jcur - Jbest) >= r*Jcur.
inline std::vector<int> run_li(const std::vector<Cycle> & sc, double t_min, double r) {
  int cur = 1; int last = -1000000;   // no lock before the first switch
  const int lock = static_cast<int>(std::lround(t_min / DT));
  std::vector<int> out; out.reserve(sc.size());
  for (size_t i = 0; i < sc.size(); ++i) {
    const auto & c = sc[i];
    if (c.safe_L && c.safe_R) {
      const int best = (c.J_L < c.J_R) ? 0 : 1;
      const double Jcur = cur == 1 ? c.J_R : c.J_L;
      const double Jbest = best == 1 ? c.J_R : c.J_L;
      if (best != cur && static_cast<int>(i) - last >= lock && (Jcur - Jbest) >= r * Jcur) {
        cur = best; last = static_cast<int>(i);
      }
    } else if (c.safe_L) cur = 0; else cur = 1;
    out.push_back(cur);
  }
  return out;
}

inline std::vector<int> run_dwell_t(const std::vector<Cycle> & sc, double t_dwell) {
  int cur = 1, lead = 1, dwell = 0;
  const int TD = static_cast<int>(std::lround(t_dwell / DT));
  std::vector<int> out; out.reserve(sc.size());
  for (const auto & c : sc) {
    int best;
    if (c.safe_L && c.safe_R) best = (c.J_L < c.J_R) ? 0 : 1;
    else if (c.safe_L) best = 0; else best = 1;
    if (best == cur) { dwell = 0; lead = cur; }
    else {
      if (best == lead) ++dwell; else { lead = best; dwell = 1; }
      if (dwell >= TD) { cur = best; dwell = 0; lead = cur; }
    }
    out.push_back(cur);
  }
  return out;
}

// Core run that also records rho after cycle `rho_at` (-1 if never reached).
inline std::vector<int> run_core_rho(const std::vector<Cycle> & sc, Knobs k,
                                     int rho_at, double * rho_out) {
  DecisionCore core(k);
  DecisionState st; st.c_star.set(PID, Side::R); st.L_plan = 1.0;
  std::vector<int> out; out.reserve(sc.size());
  if (rho_out) *rho_out = -1.0;
  for (size_t i = 0; i < sc.size(); ++i) {
    std::vector<ClassEval> evals = { mk(Side::R, sc[i].J_R, sc[i].safe_R),
                                     mk(Side::L, sc[i].J_L, sc[i].safe_L) };
    DecisionOutput o = core.step_evals(evals, st, sc[i].ttc, sc[i].v_in, i * DT, DT);
    out.push_back(o.c_star.side(PID).value() == Side::R ? 1 : 0);
    if (rho_out && static_cast<int>(i) == rho_at) *rho_out = st.rho;
  }
  return out;
}

// ---------------------------------------------------------------- policies
enum class PKind { Core, Argmin, Dwell, NoClass, CW, LI };
struct Policy {
  std::string id;      // CSV key
  std::string label;   // human-readable parameter text
  PKind kind = PKind::Core;
  Knobs knobs;         // Core
  double p1 = 0, p2 = 0;   // Dwell: t_dwell; CW: b; LI: t_min, r
};
inline std::vector<int> run_policy(const Policy & p, const std::vector<Cycle> & sc,
                                   uint32_t seed, int rho_at = -1, double * rho_out = nullptr) {
  switch (p.kind) {
    case PKind::Core:    return run_core_rho(sc, p.knobs, rho_at, rho_out);
    case PKind::Argmin:  return run_argmin(sc);
    case PKind::Dwell:   return run_dwell_t(sc, p.p1);
    case PKind::NoClass: return run_noclass(sc, seed);
    case PKind::CW:      return run_cw(sc, p.p1);
    default:             return run_li(sc, p.p1, p.p2);
  }
}

struct Run { Metrics m; int final_side; };
inline Run score(const std::vector<int> & sides) { return {compute(sides), sides.back()}; }

}  // namespace compass_eval
