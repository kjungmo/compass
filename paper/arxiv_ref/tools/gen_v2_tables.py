#!/usr/bin/env python3
"""Tables for the experiments added in v2 after review (post hoc).

Reads only the committed raw CSVs in src/compass_eval/results/v2_experiments/
and writes (--write) or verifies (--check, default):
  paper/arxiv_ref/tables/v2_native.tex  v2_native_scn.tex  v2_sigma_sw.tex
  paper/arxiv_ref/tables/v2_sigma_hit.tex  v2_tuning.tex  v2_late.tex
  src/compass_eval/results/v2_experiments/V2_results.md
Standard library only. Also imported by check_numbers.py (helper functions).
"""
import csv
import math
import re
import statistics
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
PKG = HERE.parent
ROOT = PKG.parents[1]
V2 = ROOT / "src" / "compass_eval" / "results" / "v2_experiments"

SCN = ["near_tie", "transient_spike", "mid_reversal", "clean_commit", "intermittent"]
SHORT = {"near_tie": "tie", "transient_spike": "spike", "mid_reversal": "rev",
         "clean_commit": "clean", "intermittent": "interm."}
SIGMAS = ["0.03", "0.06", "0.12", "0.2", "0.3"]
NB = ["20", "30", "40", "60"]


def _load(name):
    with (V2 / name).open(encoding="utf-8", newline="") as f:
        return list(csv.DictReader(f))


_cache = {}


def data(name):
    if name not in _cache:
        _cache[name] = _load(name)
    return _cache[name]


def sel_param(rule):
    for r in data("selected_params.csv"):
        if r["rule"] == rule:
            return r["param"], float(r["tuning_mean_loss"])
    raise KeyError(rule)


def _num(param, key):
    m = re.search(rf"{key}=([0-9.]+)", param)
    return m.group(1)


def labels():
    e0 = _num(sel_param("compass_E0")[0], "E0")
    td = _num(sel_param("dwell")[0], "T_dwell")
    b = _num(sel_param("cw")[0], "b")
    tm = _num(sel_param("li")[0], "T_min")
    r = _num(sel_param("li")[0], "r")
    return {
        "full": r"Full (proposed)",
        "hyst0": r"$-$Hysteresis",
        "hard0": r"$-$Progress hardening",
        "lambda1": r"COMPASS, $\lambda=1$",
        "compass_E0_tuned": rf"COMPASS, tuned $E_0={e0}$",
        "argmin": r"$-$Accumulator (argmin)",
        "dwell_0.6": r"Simple dwell (0.6 s)",
        "noclass": r"$-$Class correspondence",
        "dwell_tuned": rf"Simple dwell, tuned $T_{{\mathrm{{dwell}}}}={td}$ s",
        "cw_tuned": rf"CW, tuned $b={b}$",
        "li_tuned": rf"LI, tuned $T_{{\min}}={tm}$ s, $r={r}$",
    }


NATIVE_ORDER = ["full", "hyst0", "hard0", "argmin", "dwell_0.6", "noclass",
                "lambda1", "compass_E0_tuned", "dwell_tuned", "cw_tuned", "li_tuned"]
SIGMA_ORDER = ["full", "hyst0", "hard0", "lambda1", "compass_E0_tuned", "argmin",
               "dwell_0.6", "dwell_tuned", "cw_tuned", "li_tuned"]
LATE_ORDER = ["full", "hard0", "hyst0", "lambda1", "argmin", "dwell_0.6",
              "dwell_tuned", "cw_tuned", "li_tuned"]


def f(x, d):
    return f"{round(x, d):.{d}f}"


def sd(v):
    return statistics.stdev(v) if len(v) > 1 else 0.0


# ---------------------------------------------------------------- helpers for NUMBERS.md
def _stat(vals, stat):
    return {"mean": statistics.mean, "sum": sum, "max": max, "min": min, "count": len}[stat](vals)


def native(policy, scenario, metric, stat):
    rows = [r for r in data("v2_native_raw.csv")
            if r["policy"] == policy and (scenario == "*" or r["scenario"] == scenario)]
    if metric == "hit":
        rows = [r for r in rows if r["hit"] != "-1"]
    return _stat([float(r[metric]) for r in rows], stat)


def sigma(policy, sg, scenario, metric, stat):
    rows = [r for r in data("v2_sigma_raw.csv")
            if r["policy"] == policy and r["sigma"] == str(sg)
            and (scenario == "*" or r["scenario"] == scenario)]
    if metric == "hit":
        rows = [r for r in rows if r["hit"] != "-1"]
    return _stat([float(r[metric]) for r in rows], stat)


def late(policy, nb, metric, stat):
    rows = [r for r in data("v2_late_reversal_raw.csv")
            if r["policy"] == policy and r["n_burst"] == str(nb)]
    return _stat([float(r[metric]) for r in rows], stat)


def grid(split, rule, param, scenario, field):
    rows = [r for r in data("tuning_grid.csv")
            if r["split"] == split and r["rule"] == rule and r["param"] == param
            and (scenario == "*" or r["scenario"] == scenario)]
    assert rows, (split, rule, param, scenario)
    if field in ("mean_switches", "mean_excess_switches", "mean_loss"):
        return statistics.mean(float(r[field]) for r in rows)
    return sum(int(r[field]) for r in rows)


def sigma_ss(sg, lam=0.97, dt=0.05):
    """Closed-form stationary SD of the unclipped leaky accumulation (reference)."""
    return sg * dt / math.sqrt(1 - lam * lam)


# ---------------------------------------------------------------- tables
def tab_native(L):
    out = []
    for p in NATIVE_ORDER:
        rows = [r for r in data("v2_native_raw.csv") if r["policy"] == p]
        sw = [float(r["switches"]) for r in rows]
        sc = [float(r["sign_change_rate"]) for r in rows]
        en = [float(r["decision_entropy"]) for r in rows]
        tl = [float(r["t_sfx_s"]) for r in rows]
        ce = sum(int(r["censored"]) for r in rows)
        hit = sum(int(r["hit"]) for r in rows if r["hit"] != "-1")
        out.append(f"{L[p]} & {f(statistics.mean(sw),2)}$\\pm${f(sd(sw),2)} & "
                   f"{f(statistics.mean(sc),3)}$\\pm${f(sd(sc),3)} & "
                   f"{f(statistics.mean(en),3)}$\\pm${f(sd(en),3)} & "
                   f"{f(statistics.mean(tl),2)}$\\pm${f(sd(tl),2)} & {ce} & {hit} \\\\")
    return out


def tab_native_scn(L):
    out = []
    for p in NATIVE_ORDER:
        cells = []
        for s in SCN:
            rows = [r for r in data("v2_native_raw.csv") if r["policy"] == p and r["scenario"] == s]
            m = statistics.mean(float(r["switches"]) for r in rows)
            if rows[0]["hit"] == "-1":
                cells.append(f"{f(m,2)}; --")
            else:
                cells.append(f"{f(m,2)}; {sum(int(r['hit']) for r in rows)}")
        out.append(f"{L[p]} & " + " & ".join(cells) + " \\\\")
    return out


def tab_sigma_sw(L):
    out = []
    for p in SIGMA_ORDER:
        cells = []
        for sg in SIGMAS:
            cells.append(f"{f(sigma(p, sg, '*', 'switches', 'mean'), 2)} "
                         f"({f(sigma(p, sg, 'near_tie', 'switches', 'mean'), 2)})")
        out.append(f"{L[p]} & " + " & ".join(cells) + " \\\\")
    return out


def tab_sigma_hit(L):
    out = []
    for p in SIGMA_ORDER:
        cells = [f(sigma(p, sg, "*", "hit", "sum") / sigma(p, sg, "*", "hit", "count"), 2)
                 for sg in SIGMAS]
        out.append(f"{L[p]} & " + " & ".join(cells) + " \\\\")
    return out


def tab_tuning():
    names = {"compass_E0": (r"COMPASS $E_0$", r"\{0.10, 0.15, 0.20, 0.30, 0.40\}"),
             "dwell": (r"Simple dwell $T_{\mathrm{dwell}}$ (s)", r"\{0.2, 0.4, 0.6, 1.0, 1.5, 2.0\}"),
             "cw": (r"CW bonus $b$", r"\{0, 0.05, 0.10, 0.15, 0.20, 0.30, 0.40\}"),
             "li": (r"LI $(T_{\min}$ s, $r)$", r"$\{0.3, 0.6, 1, 2\}\times\{0.05, 0.10, 0.20, 0.40\}$")}
    out = []
    for rule in ["compass_E0", "dwell", "cw", "li"]:
        param, loss = sel_param(rule)
        pretty = param.replace("T_min=", "").replace("T_dwell=", "").replace("E0=", "") \
                      .replace("b=", "").replace(",r=", ", ")
        # Evaluation-seed loss of the selected point, over the same 5 scenarios.
        ev = grid("eval", rule, param, "*", "mean_loss")
        out.append(f"{names[rule][0]} & {names[rule][1]} & {pretty} & {f(loss,3)} & {f(ev,3)} \\\\")
    return out


def tab_late(L):
    out = []
    for p in LATE_ORDER:
        rho = [float(r["rho_at_burst_start"]) for r in data("v2_late_reversal_raw.csv")
               if r["policy"] == p and r["n_burst"] == "40"]
        rho_txt = f(statistics.mean(rho), 2) if rho[0] >= 0 else "--"
        cells = []
        for nb in NB:
            cells.append(f"{f(late(p, nb, 'switches', 'mean'), 2)}; "
                         f"{int(late(p, nb, 'reversed_in_burst', 'sum'))}")
        out.append(f"{L[p]} & {rho_txt} & " + " & ".join(cells) + " \\\\")
    return out


def wrap(caption, label, colspec, header, body):
    return "\n".join([
        r"\begin{table}[t]", r"\centering", f"\\caption{{{caption}}}", f"\\label{{{label}}}",
        r"\small", r"\resizebox{\linewidth}{!}{%", f"\\begin{{tabular}}{{{colspec}}}", r"\toprule",
        header, r"\midrule", *body, r"\bottomrule", r"\end{tabular}%", "}", r"\end{table}", ""])


def build():
    L = labels()
    t = {}
    t["v2_native"] = wrap(
        r"\textbf{Added in v2 after review (post hoc): comparison with two further selection rules and tuned variants, native noise.} Mean$\pm$SD over $N=250$ runs per row (5 scenarios $\times$ 50 seeds, seeds 0--49), $\sigma=0.12$ in \texttt{near\_tie} and $0.03$ elsewhere. Censored = number of the 250 runs without a reported $t_{\text{sfx}}$. Correct final side = runs whose last decision equals the scripted target side (\cref{tab:v2native_scn}), pooled over the four scenarios with a target (out of 200). The first six rows repeat the archived rows of \cref{tab:main} unchanged; no column is marked best. Tuned rows use the single procedure of \cref{tab:v2tuning}.",
        "tab:v2native", "lcccccc",
        r"Rule & Switches & Sign changes (/s) & Entropy (bits) & $t_{\text{sfx}}$ (s) & Censored & Correct final side \\",
        tab_native(L))
    t["v2_native_scn"] = wrap(
        r"\textbf{Added in v2 after review (post hoc): per scenario, mean switches per encounter; runs with the scripted final side (of 50).} Cell = mean switches; correct final side. Targets (scripted, \cref{subsec:v2}): tie none (marked --), spike R, rev L, clean L, interm.\ L. Switch counts exclude the initial class to first output, so a rule that commits on its first output shows 0 switches and is scored by the final-side count. Scenario names: tie = \texttt{near\_tie}, spike = \texttt{transient\_spike}, rev = \texttt{mid\_reversal}, clean = \texttt{clean\_commit}, interm. = \texttt{intermittent}.",
        "tab:v2native_scn", "lccccc",
        "Rule & " + " & ".join(SHORT[s] for s in SCN) + r" \\",
        tab_native_scn(L))
    t["v2_sigma_sw"] = wrap(
        r"\textbf{Added in v2 after review (post hoc): noise sweep, mean switches per encounter.} Cell = mean over the five scenarios (mean in \texttt{near\_tie}), 250 runs per cell (50 in \texttt{near\_tie}). The same $\sigma$ is used in all five scenarios. Tuned rules keep the value tuned at native noise. Reference: the stationary SD of the unclipped leaky accumulation, $\sigma\,dt/\sqrt{1-\lambda^2}$, is $0.006$, $0.012$, $0.025$, $0.041$, $0.062$ for the five $\sigma$ against $E_0=0.30$.",
        "tab:v2sigma_sw", "lccccc",
        r"Rule & $\sigma=0.03$ & 0.06 & 0.12 & 0.20 & 0.30 \\",
        tab_sigma_sw(L))
    t["v2_sigma_hit"] = wrap(
        r"\textbf{Added in v2 after review (post hoc): noise sweep, share of runs with the scripted final side.} Share of the 200 runs (four scenarios with a target $\times$ 50 seeds) whose last decision equals the target side of \cref{tab:v2native_scn}; same runs as \cref{tab:v2sigma_sw}.",
        "tab:v2sigma_hit", "lccccc",
        r"Rule & $\sigma=0.03$ & 0.06 & 0.12 & 0.20 & 0.30 \\",
        tab_sigma_hit(L))
    t["v2_tuning"] = wrap(
        r"\textbf{Added in v2 after review (post hoc): tuning procedure and selected values.} One procedure for all rules: grid search on tuning seeds 1000--1049 (disjoint from the evaluation seeds), loss = excess switches $+\,3\times$ [wrong final side] averaged over the five scenarios at native noise; ties go to the earlier grid point. The selected point is applied unchanged to the evaluation seeds. Evaluation loss uses seeds 0--49. Selected values at the edge of a grid are not extended.",
        "tab:v2tuning", "lllcc",
        r"Rule & Grid & Selected & Tuning loss & Evaluation loss \\",
        tab_tuning())
    t["v2_late"] = wrap(
        r"\textbf{Added in v2 after review (post hoc): late spurious reversal.} Scripted burst of advantage toward R for $N_b$ cycles from cycle 150, after the maneuver toward L has progressed ($v_{\text{lat}}=0.15$~m/s, $\sigma=0.03$, 50 seeds per cell). Cell = mean switches; number of the 50 runs that left L during or after the burst. $\rho$ = mean progress variable at the start of the burst (core rows only). Target: stay on L (the burst is spurious by construction).",
        "tab:v2late", "lccccc",
        r"Rule & $\rho$ at burst start & $N_b=20$ & 30 & 40 & 60 \\",
        tab_late(L))
    return t


def markdown():
    L = labels()
    md = ["# V2 experiments (post hoc) -- generated summary",
          "",
          "Generated by `paper/arxiv_ref/tools/gen_v2_tables.py` from the raw CSVs in this",
          "directory. Plan: `paper/arxiv_ref/V2_EXPERIMENT_PLAN.md`. Libstdc++ results only.",
          "Do not edit by hand.", "",
          "## Full tuning grids (mean loss per grid point; tuning seeds 1000-1049 and evaluation seeds 0-49)", "",
          "| rule | param | tuning loss | evaluation loss | evaluation excess switches | evaluation final-side hits (of 200) |",
          "|---|---|---|---|---|---|"]
    seen = []
    for r in data("tuning_grid.csv"):
        k = (r["rule"], r["param"])
        if k not in seen:
            seen.append(k)
    for rule, param in seen:
        md.append(f"| {rule} | {param} | {f(grid('tune', rule, param, '*', 'mean_loss'), 3)} | "
                  f"{f(grid('eval', rule, param, '*', 'mean_loss'), 3)} | "
                  f"{f(grid('eval', rule, param, '*', 'mean_excess_switches'), 3)} | "
                  f"{grid('eval', rule, param, '*', 'final_side_hits')} |")
    md += ["", "## Selected parameters", "", "| rule | param | tuning loss |", "|---|---|---|"]
    for r in data("selected_params.csv"):
        md.append(f"| {r['rule']} | {r['param']} | {r['tuning_mean_loss']} |")
    md += ["", "## Native noise: mean switches; final-side hits of 50 (policy x scenario)", "",
           "| policy | " + " | ".join(SCN) + " | pooled hits (of 200) | censored (of 250) |",
           "|---|" + "---|" * (len(SCN) + 2)]
    for p in NATIVE_ORDER:
        cells = []
        for s in SCN:
            m = native(p, s, "switches", "mean")
            h = "--" if native(p, s, "hit", "count") == 0 else int(native(p, s, "hit", "sum"))
            cells.append(f"{f(m,2)}; {h}")
        md.append(f"| {p} | " + " | ".join(cells) + f" | {int(native(p,'*','hit','sum'))} | "
                  f"{int(native(p,'*','censored','sum'))} |")
    md += ["", "## Noise sweep: mean switches over five scenarios (near_tie); share with final side", "",
           "| policy | " + " | ".join(f"sigma={s}" for s in SIGMAS) + " |", "|---|" + "---|" * 5]
    for p in SIGMA_ORDER:
        cells = [f"{f(sigma(p,sg,'*','switches','mean'),2)} ({f(sigma(p,sg,'near_tie','switches','mean'),2)}); "
                 f"{f(sigma(p,sg,'*','hit','sum')/sigma(p,sg,'*','hit','count'),2)}" for sg in SIGMAS]
        md.append(f"| {p} | " + " | ".join(cells) + " |")
    md += ["", "## Late spurious reversal: mean switches; runs that left L (of 50)", "",
           "| policy | rho at burst start | " + " | ".join(f"N_b={n}" for n in NB) + " |",
           "|---|---|" + "---|" * 4]
    for p in LATE_ORDER:
        rho = [float(r["rho_at_burst_start"]) for r in data("v2_late_reversal_raw.csv")
               if r["policy"] == p and r["n_burst"] == "40"]
        md.append(f"| {p} | {f(statistics.mean(rho),3) if rho[0] >= 0 else '--'} | " + " | ".join(
            f"{f(late(p,n,'switches','mean'),2)}; {int(late(p,n,'reversed_in_burst','sum'))}" for n in NB) + " |")
    return "\n".join(md) + "\n"


def main(argv):
    write = "--write" in argv
    ok = True
    outputs = {PKG / "tables" / f"{k}.tex": v for k, v in build().items()}
    outputs[V2 / "V2_results.md"] = markdown()
    for path, text in outputs.items():
        if write:
            path.write_text(text, encoding="utf-8")
        elif not path.exists() or path.read_text(encoding="utf-8") != text:
            print(f"[RED] {path.relative_to(ROOT)} differs from regenerated output")
            ok = False
    print(("wrote" if write else "[GREEN] regenerated outputs equal committed:" if ok else "")
          + f" {len(outputs)} files")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
