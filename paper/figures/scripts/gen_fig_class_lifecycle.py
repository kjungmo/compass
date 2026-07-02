#!/usr/bin/env python3
"""
fig_class_lifecycle — schematic of topological class birth/death/merge/split
with track-ID indexing and TTL, per paper Sec. 4.1.

Illustrative schematic (not measured data). Five small robot-centric scenes,
each depicting the committed class label c* as a vector of per-person L/R
passing signs indexed by track ID, and the lifecycle event that class labels
must survive: birth (new person enters), death (person leaves), transient
unavailability (TTL-held gap closure), merge (two people -> one group), and
split (group -> two people).

Usage:
    python3 gen_fig_class_lifecycle.py
Outputs:
    ../fig_class_lifecycle.pdf
    ../fig_class_lifecycle.png
"""
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Circle, FancyArrowPatch, Rectangle
from matplotlib.lines import Line2D

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.normpath(os.path.join(HERE, ".."))

ROBOT_COLOR = "#1B4F72"
PERSON_COLOR = "#B9770E"
PATH_COLOR = "#1E8449"
GHOST_COLOR = "#AAAAAA"


def draw_robot(ax, xy, r=0.16):
    ax.add_patch(Circle(xy, r, facecolor=ROBOT_COLOR, edgecolor="black",
                          linewidth=1.0, zorder=5))
    ax.text(xy[0], xy[1] - 0.34, "robot", ha="center", va="top", fontsize=6.6, color=ROBOT_COLOR)


def draw_person(ax, xy, label, color=PERSON_COLOR, alpha=1.0, r=0.14, label_offset=0.32):
    ax.add_patch(Circle(xy, r, facecolor=color, edgecolor="black",
                          linewidth=1.0, zorder=5, alpha=alpha))
    ax.text(xy[0], xy[1] + label_offset, label, ha="center", va="bottom",
             fontsize=7.2, color=color, alpha=alpha, weight="bold")


def path_arrow(ax, p0, p1, color=PATH_COLOR, style="-", lw=1.6, curve=0.0):
    a = FancyArrowPatch(p0, p1, arrowstyle="-|>", mutation_scale=11, color=color,
                          linewidth=lw, linestyle=style, zorder=4,
                          connectionstyle=f"arc3,rad={curve}")
    ax.add_patch(a)


def panel_frame(ax, title, subtitle):
    ax.set_xlim(-1.6, 1.6)
    ax.set_ylim(-1.95, 1.7)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.add_patch(Rectangle((-1.6, -1.95), 3.2, 3.65, fill=False, edgecolor="#999999", linewidth=0.8))
    ax.text(0, 1.5, title, ha="center", va="center", fontsize=9.6, weight="bold")
    ax.text(0, -1.88, subtitle, ha="center", va="bottom", fontsize=7.0, color="#444444",
             wrap=True)


def main():
    fig, axes = plt.subplots(1, 5, figsize=(15.5, 3.7))

    # ---- Panel A: birth --------------------------------------------------------
    ax = axes[0]
    panel_frame(ax, "(a) Birth", "new track #9 enters\nchallenger $c'$ starts at $e^{rev}{=}0$")
    draw_robot(ax, (0, -0.9))
    draw_person(ax, (-0.7, 0.3), "#3 (L)")
    draw_person(ax, (0.75, 0.35), "#9 (new)", color="#C0392B")
    ax.text(0.75, -0.15, "$e^{rev}{=}0$", ha="center", fontsize=6.6, color="#C0392B")
    path_arrow(ax, (0, -0.75), (-0.35, 0.55), curve=0.15)
    ax.text(-1.45, 1.1, "$c^*=$(#3:L)", fontsize=7.0, ha="left", color="#1B4F72")
    ax.text(-1.45, 0.85, "$c'=$(#3:L,#9:?)", fontsize=7.0, ha="left", color="#C0392B")

    # ---- Panel B: death ---------------------------------------------------------
    ax = axes[1]
    panel_frame(ax, "(b) Death", "track #3 exits FOV\ndim removed; $\\rho$ kept if path unchanged")
    draw_robot(ax, (0, -0.9))
    draw_person(ax, (-0.75, 0.35), "#3 (leaving)", color=GHOST_COLOR, alpha=0.45)
    draw_person(ax, (0.7, 0.3), "#12 (L)")
    path_arrow(ax, (-1.1, 0.35), (-1.5, 0.6), color=GHOST_COLOR, lw=1.2)
    path_arrow(ax, (0, -0.75), (0.35, 0.55), curve=-0.15)
    ax.text(-1.45, 1.1, "$c^*=$(#3:L,#12:L)", fontsize=6.8, ha="left", color="#1B4F72")
    ax.text(-1.45, 0.85, "$\\rightarrow c^*=$(#12:L)", fontsize=6.8, ha="left", color="#1E8449")

    # ---- Panel C: transient unavailability (TTL) ---------------------------------
    ax = axes[2]
    panel_frame(ax, "(c) Transient gap closure (TTL)", "corridor briefly $J{=}\\infty$\nidentity held for TTL, no reset")
    draw_robot(ax, (0, -0.9))
    draw_person(ax, (-0.55, 0.4), "#5 (L)")
    draw_person(ax, (0.55, 0.4), "#6 (R)")
    # occluding obstacle between them momentarily
    ax.add_patch(Rectangle((-0.18, 0.15), 0.36, 0.5, facecolor="#7B7D7D", edgecolor="black",
                             linewidth=0.8, zorder=4, hatch="//"))
    ax.text(0, 0.85, "gap closed\n(TTL held)", ha="center", fontsize=6.4, color="#7B7D7D")
    path_arrow(ax, (0, -0.75), (0, -0.05), color="#7B7D7D", lw=1.4)
    ax.text(0, -1.05, "$J(c^*){=}\\infty$ (temp.)", ha="center", fontsize=6.6, color="#943126")

    # ---- Panel D: merge -----------------------------------------------------------
    ax = axes[3]
    panel_frame(ax, "(d) Merge", "#7,#8 mutually oriented, close, static\n$\\to$ group class; 'between' pruned")
    draw_robot(ax, (0, -0.9))
    draw_person(ax, (-0.32, 0.4), "#7")
    draw_person(ax, (0.32, 0.4), "#8")
    ax.add_patch(Circle((0, 0.4), 0.42, fill=False, edgecolor="#6C3483",
                          linewidth=1.3, linestyle="--", zorder=6))
    ax.text(0, 0.95, "group {#7,#8}", ha="center", fontsize=6.8, color="#6C3483")
    path_arrow(ax, (0, -0.75), (0, -0.02), color="#6C3483", lw=1.4)
    ax.text(-1.45, 1.1, "before: (#7:L,#8:L)", fontsize=6.6, ha="left", color="#1B4F72")
    ax.text(-1.45, 0.85, "$\\rightarrow c^*=$(grp:L)", fontsize=6.6, ha="left", color="#6C3483")

    # ---- Panel E: split -------------------------------------------------------------
    ax = axes[4]
    panel_frame(ax, "(e) Split", "group separates\nchild mapped by lateral offset / heading")
    draw_robot(ax, (0, -0.9))
    draw_person(ax, (-0.62, 0.42), "#7")
    draw_person(ax, (0.62, 0.38), "#8")
    ax.add_patch(Circle((-0.62, 0.42), 0.2, fill=False, edgecolor="#6C3483",
                          linewidth=1.0, linestyle=":", zorder=3, alpha=0.6))
    path_arrow(ax, (-0.05, 0.4), (-0.5, 0.42), color="#6C3483", lw=1.1, curve=-0.3)
    path_arrow(ax, (0, -0.75), (-0.4, 0.15), color="#1E8449", lw=1.4, curve=0.1)
    ax.text(-1.45, 1.1, "before: (grp:L)", fontsize=6.8, ha="left", color="#6C3483")
    ax.text(-1.45, 0.85, "$\\rightarrow c^*=$(#7:L)", fontsize=6.8, ha="left", color="#1E8449")
    ax.text(0.62, -0.02, "(#8: new dim)", fontsize=6.2, ha="center", color="#C0392B")

    fig.suptitle(
        "Class lifecycle across encounter events (Sec. 4.1): class identity is bound to track ID, not geometry;\n"
        "TTL bridges momentary unavailability so identity survives brief gap closures",
        fontsize=10.5, y=1.04)

    legend_elems = [
        Line2D([0], [0], marker='o', color='w', markerfacecolor=ROBOT_COLOR, markersize=8, label='robot'),
        Line2D([0], [0], marker='o', color='w', markerfacecolor=PERSON_COLOR, markersize=8, label='tracked person (stable ID)'),
        Line2D([0], [0], marker='o', color='w', markerfacecolor="#C0392B", markersize=8, label='new / uncertain dimension'),
        Line2D([0], [0], marker='o', color='w', markerfacecolor=GHOST_COLOR, markersize=8, alpha=0.5, label='exiting track'),
    ]
    fig.legend(handles=legend_elems, loc="lower center", ncol=4, fontsize=8.2,
               bbox_to_anchor=(0.5, -0.06), frameon=False)

    fig.tight_layout(rect=(0, 0.02, 1, 0.94))

    pdf_path = os.path.join(OUT_DIR, "fig_class_lifecycle.pdf")
    png_path = os.path.join(OUT_DIR, "fig_class_lifecycle.png")
    fig.savefig(pdf_path, bbox_inches="tight")
    fig.savefig(png_path, dpi=220, bbox_inches="tight")
    print(f"wrote {pdf_path}")
    print(f"wrote {png_path}")


if __name__ == "__main__":
    main()
