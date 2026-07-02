#!/usr/bin/env python3
"""
fig_social_cost_field -- illustration of the asymmetric social cost field
g_i(p,t) = exp(-1/2 delta^T M_i(t)^-1 delta) around ONE moving person,
computed directly from the paper's own Sec. 4.2 formulas:

    delta = p - p_hat_i(t)
    M_i(t) = R_i(t) diag(sigma_h^2, sigma_s^2) R_i(t)^T + Sigma_i(t)

with heading-dependent sigma_h: sigma_front in the person's front half-plane,
sigma_rear in the rear half-plane (front space wider than rear -- Sec. 4.2),
sigma_s the lateral spread, and a growing prediction covariance Sigma_i(t)
added at a longer horizon time t2 > t1 (illustrating "farther future ->
wider personal-space ellipse" from the paper text).

This is an ILLUSTRATION of the cost-field model using representative
parameter values -- not measured / fitted data. It is labeled as such in
the figure.

Usage:
    python3 gen_fig_social_cost_field.py
Outputs:
    ../fig_social_cost_field.pdf
    ../fig_social_cost_field.png
"""
import os
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, Circle

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.normpath(os.path.join(HERE, ".."))


def rot(theta):
    c, s = np.cos(theta), np.sin(theta)
    return np.array([[c, -s], [s, c]])


def social_cost_field(X, Y, person_xy, heading_rad, sigma_front, sigma_rear, sigma_s, Sigma_extra):
    """
    Vectorized evaluation of g_i(p) = exp(-1/2 delta^T M_i^-1 delta) on a grid,
    with heading-dependent sigma_h (front/rear asymmetry) evaluated per grid
    point according to which half-plane (relative to the person's heading)
    that point falls in -- exactly as Sec. 4.2 specifies ("front half-plane:
    sigma_front, rear half-plane: sigma_rear", determined by projecting delta
    onto the person's local heading frame).
    """
    px, py = person_xy
    dx = X - px
    dy = Y - py

    R = rot(heading_rad)
    Rinv = R.T
    # local coords: local_h = component along heading (front axis), local_s = lateral
    local_h = Rinv[0, 0] * dx + Rinv[0, 1] * dy
    local_s = Rinv[1, 0] * dx + Rinv[1, 1] * dy

    front_mask = local_h >= 0
    sigma_h = np.where(front_mask, sigma_front, sigma_rear)

    # M_i = R diag(sigma_h^2, sigma_s^2) R^T + Sigma_extra (isotropic extra term
    # for simplicity of illustration, added to both diagonal entries in LOCAL frame)
    var_h = sigma_h ** 2 + Sigma_extra
    var_s = sigma_s ** 2 + Sigma_extra

    quad = (local_h ** 2) / var_h + (local_s ** 2) / var_s
    return np.exp(-0.5 * quad)


def draw_person_marker(ax, xy, heading_rad, color="#B9770E"):
    ax.add_patch(Circle(xy, 0.10, facecolor=color, edgecolor="black", linewidth=1.2, zorder=6))
    hx = xy[0] + 0.45 * np.cos(heading_rad)
    hy = xy[1] + 0.45 * np.sin(heading_rad)
    arr = FancyArrowPatch(xy, (hx, hy), arrowstyle="-|>", mutation_scale=14,
                            color="black", linewidth=1.6, zorder=7)
    ax.add_patch(arr)


def main():
    x = np.linspace(-3, 3, 400)
    y = np.linspace(-3, 3, 400)
    X, Y = np.meshgrid(x, y)

    person_xy = (0.0, 0.0)
    heading_deg = 35.0
    heading_rad = np.deg2rad(heading_deg)

    # illustrative parameter values (Sec. 4.2: sigma_front > sigma_rear as the
    # static asymmetry; sigma_s sets lateral spread)
    sigma_front = 1.05
    sigma_rear = 0.55
    sigma_s = 0.65

    fig, axes = plt.subplots(1, 2, figsize=(12.6, 5.4))

    horizons = [
        (0.0, "$t_1$ (near horizon): $\\Sigma_i(t_1)$ small"),
        (0.55, "$t_2 > t_1$ (far horizon): $\\Sigma_i(t_2)$ larger"),
    ]

    im = None
    for ax, (extra_var, subtitle) in zip(axes, horizons):
        Z = social_cost_field(X, Y, person_xy, heading_rad, sigma_front, sigma_rear,
                                sigma_s, extra_var)
        im = ax.contourf(X, Y, Z, levels=20, cmap="inferno", vmin=0, vmax=1)
        ax.contour(X, Y, Z, levels=[0.1, 0.3, 0.5, 0.7, 0.9], colors="white",
                    linewidths=0.6, alpha=0.7)
        draw_person_marker(ax, person_xy, heading_rad)
        ax.set_aspect("equal")
        ax.set_xlim(-3, 3)
        ax.set_ylim(-3, 3)
        ax.set_xlabel("x [m]")
        ax.set_ylabel("y [m]")
        ax.set_title(subtitle, fontsize=10)

        # annotate front/rear
        fx = person_xy[0] + 1.5 * np.cos(heading_rad)
        fy = person_xy[1] + 1.5 * np.sin(heading_rad)
        rx = person_xy[0] - 1.35 * np.cos(heading_rad)
        ry = person_xy[1] - 1.35 * np.sin(heading_rad)
        ax.text(fx, fy, "front\n($\\sigma_{front}$, wide)", color="white", fontsize=8.2,
                 ha="center", va="center", weight="bold")
        ax.text(rx, ry, "rear\n($\\sigma_{rear}$, narrow)", color="white", fontsize=8.2,
                 ha="center", va="center", weight="bold")

    fig.tight_layout(rect=(0, 0, 0.88, 0.86))

    cbar_ax = fig.add_axes([0.90, 0.14, 0.02, 0.62])
    cbar = fig.colorbar(im, cax=cbar_ax)
    cbar.set_label("$g_i(p,t)=\\exp(-\\frac{1}{2}\\delta^\\top M_i(t)^{-1}\\delta)$\n(social cost, illustrative units)",
                    fontsize=8.5)

    fig.suptitle(
        "Illustration of the asymmetric social cost field (Sec. 4.2) around one moving person\n"
        "front/rear asymmetry ($\\sigma_{front}{>}\\sigma_{rear}$) + growing prediction covariance $\\Sigma_i(t)$ at longer horizons\n"
        "(illustrative parameters -- not measured / fitted data)",
        fontsize=11)

    pdf_path = os.path.join(OUT_DIR, "fig_social_cost_field.pdf")
    png_path = os.path.join(OUT_DIR, "fig_social_cost_field.png")
    fig.savefig(pdf_path)
    fig.savefig(png_path, dpi=220)
    print(f"wrote {pdf_path}")
    print(f"wrote {png_path}")


if __name__ == "__main__":
    main()
