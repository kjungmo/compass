#!/usr/bin/env python3
"""
fig_class_lifecycle — schematic of topological class birth/death/merge/split
with track-ID indexing and TTL, per paper Sec. 4.1.

Illustrative schematic (not measured data). Five robot-centric scenes laid out
on a 2x3 grid (legend in the sixth cell), each depicting the committed class
label c* as a vector of per-person L/R passing signs indexed by track ID, and
the lifecycle event that class labels must survive: birth (new person enters),
death (person leaves), transient unavailability (TTL-held gap closure), merge
(two people -> one group), and split (group -> two people).

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
    ax.text(xy[0], xy[1] - 0.34, "robot", ha="center", va="top", fontsize=10, color=ROBOT_COLOR)


def draw_person(ax, xy, label, color=PERSON_COLOR, alpha=1.0, r=0.14, label_offset=0.30):
    ax.add_patch(Circle(xy, r, facecolor=color, edgecolor="black",
                          linewidth=1.0, zorder=5, alpha=alpha))
    ax.text(xy[0], xy[1] + label_offset, label, ha="center", va="bottom",
             fontsize=10.5, color=color, alpha=alpha, weight="bold")


def path_arrow(ax, p0, p1, color=PATH_COLOR, style="-", lw=1.8, curve=0.0):
    a = FancyArrowPatch(p0, p1, arrowstyle="-|>", mutation_scale=14, color=color,
                          linewidth=lw, linestyle=style, zorder=4,
                          connectionstyle=f"arc3,rad={curve}")
    ax.add_patch(a)


def panel_frame(ax, title, subtitle):
    ax.set_xlim(-1.6, 1.6)
    ax.set_ylim(-2.05, 1.85)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.add_patch(Rectangle((-1.6, -2.05), 3.2, 3.9, fill=False, edgecolor="#999999", linewidth=0.8))
    ax.text(0, 1.62, title, ha="center", va="center", fontsize=13, weight="bold")
    ax.text(0, -1.98, subtitle, ha="center", va="bottom", fontsize=9.5, color="#444444",
             wrap=True)


def main():
    fig, axes2d = plt.subplots(2, 3, figsize=(11.8, 8.2))
    axes = axes2d.ravel()

    # ---- Panel A: birth --------------------------------------------------------
    ax = axes[0]
    panel_frame(ax, "(a) Birth", "new track #9 enters\nchallenger $c'$ starts at $e^{rev}{=}0$")
    draw_robot(ax, (0, -0.9))
    draw_person(ax, (-0.7, 0.15), "#3 (L)")
    draw_person(ax, (0.75, 0.2), "#9 (new)", color="#C0392B")
    ax.text(0.75, -0.28, "$e^{rev}{=}0$", ha="center", fontsize=10, color="#C0392B")
    path_arrow(ax, (0, -0.75), (-0.35, 0.4), curve=0.15)
    ax.text(-1.48, 1.28, "$c^*=$(#3:L)", fontsize=10, ha="left", color="#1B4F72")
    ax.text(-1.48, 0.95, "$c'=$(#3:L,#9:?)", fontsize=10, ha="left", color="#C0392B")

    # ---- Panel B: death ---------------------------------------------------------
    ax = axes[1]
    panel_frame(ax, "(b) Death", "track #3 exits FOV\ndim removed; $\\rho$ kept if path unchanged")
    draw_robot(ax, (0, -0.9))
    draw_person(ax, (-0.72, 0.2), "#3 (leaving)", color=GHOST_COLOR, alpha=0.45)
    draw_person(ax, (0.7, 0.15), "#12 (L)")
    path_arrow(ax, (-1.05, 0.2), (-1.45, 0.45), color=GHOST_COLOR, lw=1.3)
    path_arrow(ax, (0, -0.75), (0.35, 0.4), curve=-0.15)
    ax.text(-1.48, 1.28, "$c^*=$(#3:L,#12:L)", fontsize=10, ha="left", color="#1B4F72")
    ax.text(-1.48, 0.95, "$\\rightarrow c^*=$(#12:L)", fontsize=10, ha="left", color="#1E8449")

    # ---- Panel C: transient unavailability (TTL) ---------------------------------
    ax = axes[2]
    panel_frame(ax, "(c) Transient gap closure (TTL)", "corridor briefly $J{=}\\infty$\nidentity held for TTL, no reset")
    draw_robot(ax, (0, -0.9))
    draw_person(ax, (-0.62, 0.25), "#5 (L)")
    draw_person(ax, (0.62, 0.25), "#6 (R)")
    # occluding obstacle between them momentarily
    ax.add_patch(Rectangle((-0.18, 0.0), 0.36, 0.5, facecolor="#7B7D7D", edgecolor="black",
                             linewidth=0.8, zorder=4, hatch="//"))
    ax.text(0, 0.95, "gap closed\n(TTL held)", ha="center", fontsize=9.5, color="#7B7D7D")
    path_arrow(ax, (0, -0.75), (0, -0.15), color="#7B7D7D", lw=1.5)
    ax.text(0.42, -1.35, "$J(c^*){=}\\infty$ (temp.)", ha="center", fontsize=10, color="#943126")

    # ---- Panel D: merge -----------------------------------------------------------
    ax = axes[3]
    panel_frame(ax, "(d) Merge", "#7,#8 mutually oriented, close, static\n$\\to$ group class; 'between' pruned")
    draw_robot(ax, (0, -0.9))
    draw_person(ax, (-0.32, 0.2), "#7", label_offset=-0.55)
    draw_person(ax, (0.32, 0.2), "#8", label_offset=-0.55)
    ax.add_patch(Circle((0, 0.2), 0.44, fill=False, edgecolor="#6C3483",
                          linewidth=1.4, linestyle="--", zorder=6))
    ax.text(0, 0.78, "group {#7,#8}", ha="center", fontsize=10, color="#6C3483")
    path_arrow(ax, (0, -0.75), (0, -0.3), color="#6C3483", lw=1.5)
    ax.text(-1.48, 1.28, "before: (#7:L,#8:L)", fontsize=10, ha="left", color="#1B4F72")
    ax.text(-1.48, 0.95, "$\\rightarrow c^*=$(grp:L)", fontsize=10, ha="left", color="#6C3483")

    # ---- Panel E: split -------------------------------------------------------------
    ax = axes[4]
    panel_frame(ax, "(e) Split", "group separates\nchild mapped by lateral offset / heading")
    draw_robot(ax, (0, -0.9))
    draw_person(ax, (-0.62, 0.28), "#7", label_offset=0.28)
    draw_person(ax, (0.72, 0.24), "#8", label_offset=0.28)
    ax.add_patch(Circle((-0.62, 0.28), 0.22, fill=False, edgecolor="#6C3483",
                          linewidth=1.1, linestyle=":", zorder=3, alpha=0.6))
    path_arrow(ax, (-0.02, 0.26), (-0.46, 0.28), color="#6C3483", lw=1.2, curve=-0.3)
    path_arrow(ax, (0, -0.75), (-0.4, 0.0), color="#1E8449", lw=1.5, curve=0.1)
    ax.text(-1.48, 1.28, "before: (grp:L)", fontsize=10, ha="left", color="#6C3483")
    ax.text(-1.48, 0.95, "$\\rightarrow c^*=$(#7:L)", fontsize=10, ha="left", color="#1E8449")
    ax.text(0.72, -0.35, "(#8: new dim)", fontsize=9.5, ha="center", color="#C0392B")

    # ---- Legend cell ---------------------------------------------------------------
    ax = axes[5]
    ax.axis("off")
    legend_elems = [
        Line2D([0], [0], marker='o', color='w', markerfacecolor=ROBOT_COLOR, markersize=13, label='robot'),
        Line2D([0], [0], marker='o', color='w', markerfacecolor=PERSON_COLOR, markersize=13, label='tracked person (stable ID)'),
        Line2D([0], [0], marker='o', color='w', markerfacecolor="#C0392B", markersize=13, label='new / uncertain dimension'),
        Line2D([0], [0], marker='o', color='w', markerfacecolor=GHOST_COLOR, markersize=13, alpha=0.5, label='exiting track'),
    ]
    ax.legend(handles=legend_elems, loc="center", fontsize=11.5, frameon=False,
              borderaxespad=0, handletextpad=0.6, labelspacing=1.0)

    fig.suptitle(
        "Class lifecycle across encounter events (Sec. 4.1): class identity is bound to track ID, not geometry;\n"
        "TTL bridges momentary unavailability so identity survives brief gap closures",
        fontsize=13.5, y=0.995)

    fig.tight_layout(rect=(0, 0.0, 1, 0.93))

    pdf_path = os.path.join(OUT_DIR, "fig_class_lifecycle.pdf")
    png_path = os.path.join(OUT_DIR, "fig_class_lifecycle.png")
    fig.savefig(pdf_path, bbox_inches="tight")
    fig.savefig(png_path, dpi=220, bbox_inches="tight")
    print(f"wrote {pdf_path}")
    print(f"wrote {png_path}")


if __name__ == "__main__":
    main()
