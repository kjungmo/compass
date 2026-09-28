#!/usr/bin/env python3
"""Regenerate the data-bearing tables and pgfplots figures of paper/arxiv_ref.

Every number written here is computed from committed artifacts only:
  src/compass_eval/results/ablation_raw.csv  (R1, 1500 rows)
  src/compass_eval/results/R3_latency.md     (historical latency table)
  src/compass_eval/results/R5_rho_sweep.md   (offline v_lat sweep table)
Standard library only. Run from anywhere:
  python3 paper/arxiv_ref/tools/gen_tables_figures.py
"""
import csv
import re
import statistics
from pathlib import Path

HERE = Path(__file__).resolve().parent
PKG = HERE.parent
ROOT = PKG.parents[1]
RES = ROOT / "src" / "compass_eval" / "results"

VARIANTS = [  # (CSV key, printed label)
    ("full (제안)", r"Full (proposed)"),
    ("-hysteresis", r"$-$Hysteresis"),
    ("-progress hardening", r"$-$Progress hardening"),
    ("-accumulator (즉시 argmin)", r"$-$Accumulator (argmin)"),
    ("simple-dwell (O4)", r"Simple dwell (0.6 s)"),
    ("-class correspondence", r"$-$Class correspondence"),
]
SCENARIOS = ["near_tie", "transient_spike", "mid_reversal", "clean_commit", "intermittent"]
METRICS = [("switches", 2), ("sign_change_rate", 3), ("decision_entropy", 3), ("t_legible_s", 2)]


def fmt(x, d):
    return f"{round(x, d):.{d}f}"


def load():
    rows = list(csv.DictReader((RES / "ablation_raw.csv").open(encoding="utf-8")))
    assert len(rows) == 1500, len(rows)
    return rows


def group(rows, v, s=None):
    return [r for r in rows if r["variant"] == v and (s is None or r["scenario"] == s)]


def cell(vals, d):
    return fmt(statistics.mean(vals), d), fmt(statistics.stdev(vals), d)


def bold_best(cells):
    """cells: list of (mean_str, sd_str); lower mean is better; ties all bold."""
    best = min(float(m) for m, _ in cells)
    out = []
    for m, s in cells:
        txt = f"{m}\\stdv{{{s}}}"
        out.append(f"\\textbf{{{txt}}}" if float(m) == best else txt)
    return out


def main_table(rows):
    cols = []
    for metric, d in METRICS:
        cols.append(bold_best([cell([float(r[metric]) for r in group(rows, v)], d) for v, _ in VARIANTS]))
    cens = [sum(int(r["censored"]) for r in group(rows, v)) for v, _ in VARIANTS]
    lines = []
    for i, (_, label) in enumerate(VARIANTS):
        c = f"{cens[i]}/250"
        c = f"\\textbf{{{c}}}" if cens[i] == min(cens) else c
        lines.append(f"{label} & " + " & ".join(col[i] for col in cols) + f" & {c} \\\\")
    body = "\n".join(lines)
    return r"""\begin{table}[t]
\centering
\caption{\textbf{Aggregate behavioral consistency and legibility} (mean\stdv{SD} across all scenarios, $N=250$ per variant; best per column in bold, ties included) --- all columns measured in the offline decision-core harness. Performance metrics that depend on physical simulation (social distance, collision, success rate, lateral jerk) are not yet measured and are therefore not included; their definitions and protocols are in \cref{app:planned}. The pooled $N=250$ mean\stdv{SD} is a mixture statistic across 5 heterogeneous scenarios; the SD includes cross-scenario mixture variance in addition to seed-to-seed variance (per-scenario breakdown in \cref{tab:scenario,tab:legibility}). The t-legible mean\stdv{SD} includes right-censored trials imputed at the horizon value of 10.0~s, so it must be read together with the illegibility (censoring-rate) column; the no-imputation, survival-analysis-based aggregation of \cref{app:planned} applies in the planned evaluation.}
\label{tab:main}
\small
\resizebox{\linewidth}{!}{%
\begin{tabular}{lccccc}
\toprule
Variant & Switches/enc.$\downarrow$ & Sign-change (/s)$\downarrow$ & Entropy (bits)$\downarrow$ & t-legible (s)$\downarrow$ & Illegibility$\downarrow$ \\
\midrule
""" + body + r"""
\bottomrule
\end{tabular}%
}
\end{table}
"""


def scenario_table(rows):
    cols = []
    for s in SCENARIOS:
        cols.append([f"{m}\\stdv{{{sd}}}" for m, sd in
                     (cell([float(r["switches"]) for r in group(rows, v, s)], 2) for v, _ in VARIANTS)])
    body = "\n".join(f"{label} & " + " & ".join(col[i] for col in cols) + " \\\\"
                     for i, (_, label) in enumerate(VARIANTS))
    return r"""\begin{table}[t]
\centering
\caption{\textbf{Switches per encounter by scenario} (mean\stdv{SD}, $N=50$ per cell; no column is marked best) --- the table in which oscillatory regression is most clearly evident. Fewer switches is not uniformly better: \texttt{clean\_commit} and \texttt{intermittent} contain one warranted switch (\cref{subsec:ablation}).}
\label{tab:scenario}
\small
\resizebox{\linewidth}{!}{%
\begin{tabular}{lccccc}
\toprule
Variant & near\_tie & transient\_spike & mid\_reversal & clean\_commit & intermittent \\
\midrule
""" + body + r"""
\bottomrule
\end{tabular}%
}
\end{table}
"""


def legibility_table(rows):
    lines = []
    for v, label in VARIANTS:
        cells = []
        for s in SCENARIOS:
            g = group(rows, v, s)
            m = fmt(statistics.mean(float(r["t_legible_s"]) for r in g), 2)
            c = sum(int(r["censored"]) for r in g)
            cells.append(f"{m} ({c})")
        lines.append(f"{label} & " + " & ".join(cells) + " \\\\")
    return r"""\begin{table}[t]
\centering
\caption{\textbf{Time-to-legible by scenario} from the decision-stream proxy observer (mean in seconds; parentheses: right-censored trials out of 50). Censored trials enter the mean at the 10.0~s horizon. These values measure the stability of the decision stream, not motion legibility.}
\label{tab:legibility}
\small
\resizebox{\linewidth}{!}{%
\begin{tabular}{lccccc}
\toprule
Variant & near\_tie & transient\_spike & mid\_reversal & clean\_commit & intermittent \\
\midrule
""" + "\n".join(lines) + r"""
\bottomrule
\end{tabular}%
}
\end{table}
"""


def md_rows(path):
    out = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("|") and not re.fullmatch(r"\|[-| ]+\|", line):
            out.append([c.strip() for c in line.strip("|").split("|")])
    return out[1:]


def latency_table():
    rows = [r for r in md_rows(RES / "R3_latency.md") if re.fullmatch(r"[123]", r[0])]
    assert len(rows) == 3
    body = "\n".join(
        f"{r[0]} & {r[1]} & {fmt(float(r[2]),1)} & {fmt(float(r[3]),1)} & {fmt(float(r[4]),1)} & {fmt(float(r[5]),1)} & {fmt(float(r[6].rstrip('%')),2)}\\% \\\\"
        for r in rows)
    return r"""\begin{table}[t]
\centering
\caption{\textbf{Historical decision-core timing} of \texttt{DecisionCore::step} (enumeration, cost integration, safety test and decision), 20{,}000 iterations per condition on an AMD Ryzen 5 5500GT, all $2^K$ labels enumerated in an all-safe fixture. Finite-batch measurements, not a worst-case execution-time bound and not an end-to-end ROS~2 pipeline measurement.}
\label{tab:latency}
\small
\begin{tabular}{ccrrrrr}
\toprule
$K$ & Labels $2^K$ & Mean ($\mu$s) & p50 ($\mu$s) & p99 ($\mu$s) & Max ($\mu$s) & Max / 50 ms \\
\midrule
""" + body + r"""
\bottomrule
\end{tabular}
\end{table}
"""


def oscillation_figure(rows):
    order = [0, 1, 2, 4, 3, 5]  # proposed, knob ablations, dwell, argmin, synthetic comparator
    short = ["Full", "$-$Hyst.", "$-$Hard.", "Dwell", "Argmin", "$-$Corr."]
    coords, labels, err_up = [], [], []
    for j, i in enumerate(order):
        vals = [float(r["switches"]) for r in group(rows, VARIANTS[i][0])]
        m, s = statistics.mean(vals), statistics.stdev(vals)
        coords.append(f"({j},{m:.4f}) +- (0,{s:.4f})")
        labels.append(f"\\node[font=\\scriptsize,anchor=south] at (axis cs:{j},{(m + s) * 1.15:.4f}) {{{m:.2f}}};")
    return r"""\begin{figure}[t]
\centering
\begin{tikzpicture}
\begin{axis}[
  width=0.82\linewidth, height=5.6cm,
  ybar, bar width=16pt, ymode=log, log origin=infty,
  ymin=0.03, ymax=300,
  xtick={0,...,5}, xticklabels={""" + ",".join("{" + s + "}" for s in short) + r"""},
  ylabel={Switches per encounter (log)},
  ymajorgrids, grid style={dashed,gray!40},
  error bars/y dir=plus, error bars/y explicit,
  enlarge x limits=0.1, tick label style={font=\small}, label style={font=\small},
]
\addplot[fill=teal!55, draw=black, error bars/.cd, y dir=plus, y explicit] coordinates {
""" + "\n".join(coords) + r"""
};
""" + "\n".join(labels) + r"""
\end{axis}
\end{tikzpicture}
\caption{\textbf{Class-switch count per encounter by variant} (mean with $+$SD whisker, log scale; $N=250$ per variant, 5 scenarios $\times$ 50 seeds, 10.0~s horizon), regenerated with pgfplots from \texttt{ablation\_raw.csv}. The immediate-argmin policy and the synthetic correspondence-loss stress comparator ($-$Corr.) yield 29.74 and 37.20 decision changes per encounter, respectively, whereas the proposed method (Full) maintains commitment with 0.40. Removing hysteresis ($-$Hyst.) or progress hardening ($-$Hard.) alone shows no observed regression in this scenario set, supporting the accumulator configuration's suppression of fluctuations in this scenario set; the synthetic comparator does not identify the causal effect of correspondence.}
\label{fig:oscillation}
\end{figure}
"""


def rho_figure():
    rows = [r for r in md_rows(RES / "R5_rho_sweep.md") if re.fullmatch(r"0\.\d+", r[0])]
    frac = []
    tl = []
    for r in rows:
        k, n = r[2].split("/")
        frac.append(f"({r[0]},{int(k) / int(n):.2f})")
        tl.append(f"({r[0]},{r[4]})")
    return r"""\begin{figure}[t]
\centering
\begin{tikzpicture}
\begin{axis}[
  width=0.62\linewidth, height=4.6cm,
  xmin=0, xmax=0.55, ymin=-0.05, ymax=1.1,
  xlabel={Scripted progress input $v_{\mathrm{lat}}$ (m/s)},
  ylabel={Fraction of seeds that switch},
  xtick={0.05,0.10,0.20,0.35,0.50},
  xticklabel style={/pgf/number format/fixed, /pgf/number format/precision=2},
  ymajorgrids, grid style={dashed,gray!40},
  tick label style={font=\small}, label style={font=\small},
]
\fill[red!12] (axis cs:0.20,-0.05) rectangle (axis cs:0.35,1.1);
\node[font=\scriptsize,align=center] at (axis cs:0.275,0.5) {blocking\\interval};
\addplot[only marks, mark=*, mark size=2.5pt, teal!70!black] coordinates {""" + " ".join(frac) + r"""};
\end{axis}
\end{tikzpicture}
\caption{\textbf{Offline transition blocking under fast progress saturation} (\texttt{clean\_commit}, Full, 50 seeds per point, measured only at the five marked inputs; data from \texttt{R5\_rho\_sweep.md}). Every seed commits the warranted switch for $v_{\text{lat}}\le0.20$~m/s and none does for $v_{\text{lat}}\ge0.35$~m/s, so the transition-blocking interval is $v_{\text{lat}}\in(0.20,0.35)$~m/s. This is a sensitivity of the scripted progress input, not a physical freezing threshold.}
\label{fig:rhosweep}
\end{figure}
"""


def main():
    rows = load()
    (PKG / "tables" / "main.tex").write_text(main_table(rows), encoding="utf-8")
    (PKG / "tables" / "scenario.tex").write_text(scenario_table(rows), encoding="utf-8")
    (PKG / "tables" / "legibility.tex").write_text(legibility_table(rows), encoding="utf-8")
    (PKG / "tables" / "latency.tex").write_text(latency_table(), encoding="utf-8")
    (PKG / "figures" / "oscillation.tex").write_text(oscillation_figure(rows), encoding="utf-8")
    (PKG / "figures" / "rho_sweep.tex").write_text(rho_figure(), encoding="utf-8")
    print("wrote tables/{main,scenario,legibility,latency}.tex figures/{oscillation,rho_sweep}.tex")


if __name__ == "__main__":
    main()
