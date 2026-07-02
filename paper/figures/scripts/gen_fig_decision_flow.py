#!/usr/bin/env python3
"""
fig_decision_flow — decision-layer architecture / flow diagram.

Illustrative schematic (not measured data). Faithful to Sec. 4.5 pseudocode
(the integrated algorithm) and Sec. 4.3 commitment-switching rule as
restructured around a SINGLE challenger accumulator e^rev (the e^fwd
"forward accumulator" construct has been removed from the paper because it
had no update law / was dead state -- see the audit notes). This diagram
therefore draws only one leaky-evidence accumulator, for the challenger
class c', with the switching test e^rev >= E_th(rho) = E_0(1 + k_rho rho^p).

Usage:
    python3 gen_fig_decision_flow.py
Outputs:
    ../fig_decision_flow.pdf
    ../fig_decision_flow.png
"""
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch
from matplotlib.lines import Line2D

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.normpath(os.path.join(HERE, ".."))

BOX_STYLE = dict(boxstyle="round,pad=0.35,rounding_size=0.12", linewidth=1.2)


def box(ax, xy, w, h, text, facecolor="#EAF2F8", edgecolor="#1B4F72", fontsize=12.1, weight="normal"):
    x, y = xy
    patch = FancyBboxPatch((x, y), w, h, facecolor=facecolor, edgecolor=edgecolor,
                            linewidth=1.3, boxstyle="round,pad=0.02,rounding_size=0.08",
                            zorder=3)
    ax.add_patch(patch)
    ax.text(x + w / 2, y + h / 2, text, ha="center", va="center",
             fontsize=fontsize, weight=weight, zorder=4, linespacing=1.35)
    return (x, y, w, h)


def arrow(ax, p0, p1, color="#333333", lw=1.4, style="-|>", connectionstyle="arc3,rad=0.0", ls="-"):
    a = FancyArrowPatch(p0, p1, arrowstyle=style, mutation_scale=13,
                          color=color, linewidth=lw, zorder=2,
                          connectionstyle=connectionstyle, linestyle=ls)
    ax.add_patch(a)


def right_mid(b):
    x, y, w, h = b
    return (x + w, y + h / 2)


def left_mid(b):
    x, y, w, h = b
    return (x, y + h / 2)


def top_mid(b):
    x, y, w, h = b
    return (x + w / 2, y + h)


def bottom_mid(b):
    x, y, w, h = b
    return (x + w / 2, y)


def main():
    fig, ax = plt.subplots(figsize=(11.9, 7.6))
    ax.set_xlim(-0.6, 11.5)
    ax.set_ylim(0, 9.7)
    ax.axis("off")

    ax.text(5.75, 9.45, "COMPASS decision-layer flow (per control cycle $k$)",
             ha="center", va="center", fontsize=17.2, weight="bold")

    # --- Row 1: sensing / inputs -------------------------------------------------
    b_people = box(ax, (0.3, 7.55), 2.55, 1.05,
                    "Tracked people\n/people (track-ID, pose, vel)",
                    facecolor="#FDEBD0", edgecolor="#B9770E")
    b_costmap = box(ax, (3.15, 7.55), 2.35, 1.05,
                      "Local costmap\n(static obstacles, gaps)",
                      facecolor="#FDEBD0", edgecolor="#B9770E")
    b_plan = box(ax, (5.8, 7.55), 2.2, 1.05,
                  "Global plan\n(local goal $g$)",
                  facecolor="#FDEBD0", edgecolor="#B9770E")
    b_predict = box(ax, (8.3, 7.55), 2.9, 1.05,
                      "Constant-velocity\nKalman prediction\n$\\{\\hat{p}_i(t), \\Sigma_i(t)\\}$",
                      facecolor="#FDEBD0", edgecolor="#B9770E")

    # --- Row 2: class enumeration -------------------------------------------------
    b_enum = box(ax, (0.3, 5.95), 5.05, 1.15,
                  "Class enumeration + track-ID lifecycle (Sec. 4.1)\n"
                  "$c=(s_{id_1},\\ldots,s_{id_K})$, $s\\in\\{L,R\\}$ ; TTL-gated birth/death/merge/split\n"
                  "gap-width pruning $\\Rightarrow J=\\infty$",
                  facecolor="#EBF5FB", edgecolor="#1F618D", fontsize=10.8)
    b_cost = box(ax, (5.5, 5.95), 5.7, 1.15,
                  "Cost $J(c)$ per class (Sec. 4.2)\n"
                  "$J = w_g \\hat J_{goal} + w_s \\hat J_{social} + w_e \\hat J_{effort} + w_r \\hat J_{rule}$\n"
                  "hard availability: $d_{obs}{<}r_{safe}$, gap, kinematics, $d_{safe} \\Rightarrow J=\\infty$",
                  facecolor="#EBF5FB", edgecolor="#1F618D", fontsize=11.6)

    arrow(ax, right_mid(b_enum), left_mid(b_cost))

    # arrows from the four inputs down into enum/cost row
    arrow(ax, bottom_mid(b_people), (2.2, 7.1))
    arrow(ax, bottom_mid(b_costmap), (3.0, 7.1))
    arrow(ax, bottom_mid(b_plan), (7.5, 7.1))
    arrow(ax, bottom_mid(b_predict), (9.5, 7.1))

    # --- Row 3: commitment switching (single challenger accumulator) -------------
    b_switch = box(ax, (0.3, 3.35), 6.35, 2.15,
                    "Leaky-evidence commitment switching (Sec. 4.3, Sec. 4.5 step 3)\n\n"
                    "challenger $c'_k=\\mathrm{TIEBREAK}(\\arg\\min_{c\\neq c^*} J_k(c))$\n"
                    "advantage $D_k = J_k(c^*_{k-1}) - J_k(c'_k)$\n"
                    "if $c' \\neq \\mathrm{prev}\\,c'$: $e^{rev}\\!\\leftarrow 0$\n"
                    "$e^{rev}_k=\\mathrm{clip}(\\lambda e^{rev}_{k-1}+(D_k-\\Delta_{floor})dt,\\,0,\\,e^{rev}_{max})$\n"
                    "switch iff  $e^{rev}_k \\geq E_{th}(\\rho_k) = E_0(1+k_\\rho\\rho_k^{\\,p})$",
                    facecolor="#EAFAF1", edgecolor="#1E8449", fontsize=11.6)

    b_dwell = box(ax, (6.95, 3.35), 2.15, 1.0,
                   "Dwell / hysteresis\n($E_0$, tie-break\n$\\varepsilon_{in}{<}\\varepsilon_{out}$)",
                   facecolor="#EAFAF1", edgecolor="#1E8449", fontsize=10.8)
    b_rho = box(ax, (6.95, 4.5), 3.05, 1.0,
                 "Progress hardening\n$\\rho_k=\\mathrm{clip}(\\mathrm{realized}/\\mathrm{planned},0,1)$",
                 facecolor="#EAFAF1", edgecolor="#1E8449", fontsize=10.8)

    arrow(ax, bottom_mid(b_cost), (bottom_mid(b_cost)[0], 5.5))
    arrow(ax, (8.35, 5.95), (4.2, 5.5), connectionstyle="arc3,rad=0.15")
    arrow(ax, right_mid(b_dwell), left_mid(b_switch), connectionstyle="arc3,rad=0.0")
    arrow(ax, right_mid(b_rho), left_mid(b_switch), connectionstyle="arc3,rad=0.0")

    # --- Row 4: safety override (lexicographic, overrides everything) ------------
    b_safety = box(ax, (0.3, 1.15), 6.35, 1.75,
                    "Lexicographic safety override (Sec. 4.4, Sec. 4.5 step 2)\n\n"
                    "if $c^* \\notin \\mathcal{S}$ (unsafe): ignore accumulator / dwell entirely\n"
                    "1. switch to safe class  2. brake  3. stop / HOLD (thrash guard $N_{thrash}$)",
                    facecolor="#FDEDEC", edgecolor="#943126", fontsize=11.9)

    arrow(ax, bottom_mid(b_switch), (bottom_mid(b_switch)[0], 2.9))
    ax.text(3.7, 3.05, "commit $c^*\\!\\leftarrow c'$, $e^{rev}\\!\\leftarrow 0$, $\\rho\\!\\leftarrow 0$",
             ha="center", va="center", fontsize=10.0, style="italic", color="#1E8449")

    # --- Row 5: trajectory interface ----------------------------------------------
    b_traj = box(ax, (0.3, 0.15), 6.35, 0.75,
                  "Trajectory interface $\\rightarrow$ layer ② :  $\\{c^*,\\ \\text{corridor}(c^*),\\ v_{target}\\}$",
                  facecolor="#F4ECF7", edgecolor="#6C3483", fontsize=12.1)
    arrow(ax, bottom_mid(b_safety), (bottom_mid(b_safety)[0], 0.9))

    # side note: safety overrides the switching block directly (bypass)
    arrow(ax, (0.05, 4.4), (0.05, 2.0), color="#943126", lw=1.6,
          connectionstyle="arc3,rad=0.0")
    ax.text(-0.30, 3.2, "safety\npriority\n(overrides)", rotation=90, ha="center", va="center",
             fontsize=10.0, color="#943126", weight="bold")
    arrow(ax, (0.05, 2.0), (0.5, 1.9), color="#943126", lw=1.6, connectionstyle="arc3,rad=-0.2")

    # note box: removed e^fwd construct, single accumulator design
    note_x, note_y, note_w, note_h = 7.35, 1.15, 3.85, 1.75
    note = FancyBboxPatch((note_x, note_y), note_w, note_h, facecolor="#FEF9E7",
                            edgecolor="#B7950B", linewidth=1.0, linestyle="--",
                            boxstyle="round,pad=0.02,rounding_size=0.08", zorder=3)
    ax.add_patch(note)
    ax.text(note_x + note_w / 2, note_y + note_h / 2,
             "Single-accumulator design:\nonly the challenger accumulator\n"
             "$e^{rev}$ is tracked. Switching-rate\nbound holds for\n"
             "$E_0 < e^{rev}_{max} \\leq E_0(1+k_\\rho)$\n(liveness of $e^{rev}$, Sec. 4.6).",
             ha="center", va="center", fontsize=10.6, linespacing=1.4)

    fig.tight_layout()
    pdf_path = os.path.join(OUT_DIR, "fig_decision_flow.pdf")
    png_path = os.path.join(OUT_DIR, "fig_decision_flow.png")
    fig.savefig(pdf_path)
    fig.savefig(png_path, dpi=220)
    print(f"wrote {pdf_path}")
    print(f"wrote {png_path}")


if __name__ == "__main__":
    main()
