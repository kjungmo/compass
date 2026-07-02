#!/usr/bin/env python3
"""
fig_oscillation_compare — per-variant oscillation counts (ablation study).

Data source (REAL, measured): src/compass_eval/results/ablation_raw.csv
Columns: variant,scenario,seed,switches,sign_change_rate,decision_entropy,t_legible_s,censored

This script aggregates the `switches` column (class-switch count per encounter,
over a 10.0 s / 200-cycle horizon) per ablation variant across all 5 scenarios
(250 runs/variant total: 5 scenarios x 50 seeds), and renders a log-scale bar
chart with mean +/- SD error bars. No numbers are invented here -- everything
is computed directly from the committed CSV.

Usage:
    python3 gen_fig_oscillation_compare.py
Outputs:
    ../fig_oscillation_compare.pdf
    ../fig_oscillation_compare.png
"""
import csv
import os
import math
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CSV_PATH = os.path.normpath(
    os.path.join(HERE, "..", "..", "..", "src", "compass_eval", "results", "ablation_raw.csv")
)
OUT_DIR = os.path.normpath(os.path.join(HERE, ".."))

# Korean variant label (as it appears in the CSV) -> English label for the figure.
# English labels are used because the paper is being submitted in English (arXiv).
VARIANT_LABEL_EN = {
    "full (제안)": "Full (proposed)",
    "-hysteresis": "No hysteresis",
    "-progress hardening": "No progress hardening",
    "-accumulator (즉시 argmin)": "No accumulator (immediate argmin)",
    "simple-dwell (O4)": "Simple dwell timer",
    "-class correspondence": "No class correspondence",
}

# Fixed left-to-right order: proposed method first, then ablations, then the O4 baseline.
ORDER = [
    "full (제안)",
    "-hysteresis",
    "-progress hardening",
    "simple-dwell (O4)",
    "-accumulator (즉시 argmin)",
    "-class correspondence",
]


def load_data(csv_path):
    per_variant = {}
    with open(csv_path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            v = row["variant"]
            per_variant.setdefault(v, []).append(float(row["switches"]))
    return per_variant


def main():
    per_variant = load_data(CSV_PATH)

    missing = [v for v in ORDER if v not in per_variant]
    if missing:
        raise SystemExit(f"variants missing from CSV: {missing}")

    means, sds, ns = [], [], []
    for v in ORDER:
        vals = np.array(per_variant[v], dtype=float)
        means.append(vals.mean())
        sds.append(vals.std(ddof=1))
        ns.append(len(vals))

    labels = [VARIANT_LABEL_EN[v] for v in ORDER]

    # Small floor so the log-scale bars for near-zero means (e.g. 0.40) still
    # render visibly; the floor is only a plotting device, not a data change.
    plot_floor = 0.05

    fig, ax = plt.subplots(figsize=(8.4, 4.8))

    colors = ["#2E7D32"] + ["#6699CC"] * 3 + ["#C0392B", "#C0392B"]
    # full(proposed)=green, hysteresis/progress/O4-dwell ablations=blue,
    # the two catastrophic ablations (no accumulator, no class correspondence)=red
    colors = [
        "#2E7D32",  # full (proposed)
        "#4C78A8",  # -hysteresis
        "#4C78A8",  # -progress hardening
        "#8C8C8C",  # simple-dwell (O4)
        "#C0392B",  # -accumulator
        "#C0392B",  # -class correspondence
    ]

    x = np.arange(len(labels))
    plot_vals = [max(m, plot_floor) for m in means]
    bars = ax.bar(x, plot_vals, yerr=sds, capsize=4, color=colors,
                   edgecolor="black", linewidth=0.6, zorder=3,
                   error_kw={"elinewidth": 1.0, "ecolor": "#333333"})

    ax.set_yscale("log")
    ax.set_ylim(0.03, 200)
    ax.set_ylabel("Class switches per encounter\n(mean $\\pm$ SD, log scale)", fontsize=12)
    ax.set_xticks(x)
    ax.set_xticklabels(labels, fontsize=11, rotation=18, ha="right", rotation_mode="anchor")
    ax.tick_params(axis="y", labelsize=11)
    ax.set_title("Ablation: passing-class oscillation count\n(N=250 runs/variant, 5 scenarios x 50 seeds, 10.0 s horizon)",
                  fontsize=11)
    ax.grid(axis="y", which="major", linestyle="--", linewidth=0.5, alpha=0.6, zorder=0)
    ax.grid(axis="y", which="minor", linestyle=":", linewidth=0.3, alpha=0.3, zorder=0)

    for xi, m, s, n in zip(x, means, sds, ns):
        # Place the value label above the error-bar cap (mean + SD), not the bar
        # top, so it never collides with the upper whisker.
        y_label = max(m + s, max(m, plot_floor)) * 1.15
        ax.text(xi, y_label, f"{m:.2f}",
                ha="center", va="bottom", fontsize=8.5)

    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)

    fig.text(0.5, 0.005,
              "Source: src/compass_eval/results/ablation_raw.csv (offline decision-core harness, measured)",
              ha="center", fontsize=6.5, color="#555555")

    fig.tight_layout(rect=(0, 0.05, 1, 1))

    pdf_path = os.path.join(OUT_DIR, "fig_oscillation_compare.pdf")
    png_path = os.path.join(OUT_DIR, "fig_oscillation_compare.png")
    fig.savefig(pdf_path)
    fig.savefig(png_path, dpi=220)
    print(f"wrote {pdf_path}")
    print(f"wrote {png_path}")

    print("\nAggregated means (switches/encounter, N=250 each):")
    for v, m, s in zip(ORDER, means, sds):
        print(f"  {v!r}: mean={m:.3f} sd={s:.3f}")


if __name__ == "__main__":
    main()
