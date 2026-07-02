<div align="center">

# COMPASS

**COMmitment-based PASSing for Social navigation — temporally consistent
left/right passing decisions for service robots, via leaky-evidence
commitment switching under a lexicographic safety override.**

[![License](https://img.shields.io/github/license/kjungmo/compass?color=blue)](LICENSE)
![ROS 2 Jazzy](https://img.shields.io/badge/ROS_2-Jazzy-22314E?logo=ros&logoColor=white)
![Gazebo Harmonic](https://img.shields.io/badge/Gazebo-Harmonic-f58113)
[![Paper](https://img.shields.io/badge/Paper-PDF-b31b1b)](paper/arxiv/main.pdf)
[![Sponsor](https://img.shields.io/github/sponsors/kjungmo?logo=githubsponsors&color=ea4aaa)](https://github.com/sponsors/kjungmo)

[Overview](#overview) &nbsp;·&nbsp; [Paper](#-paper) &nbsp;·&nbsp;
[Build](#-build--test) &nbsp;·&nbsp; [Results](#-measured-results) &nbsp;·&nbsp;
[Testbed](#-gazebo-testbed) &nbsp;·&nbsp; [Docs](#-documentation) &nbsp;·&nbsp;
[Roadmap](#-roadmap)

<img src="paper/figures/fig_decision_flow.png" width="640" alt="COMPASS decision-layer flow per control cycle: class enumeration bound to track IDs, per-class cost, leaky-evidence commitment switching, and a lexicographic safety override feeding the trajectory interface"/>

</div>

Service robots in pedestrian spaces re-decide every control cycle which side —
left or right — to pass a person on. When that re-decision is greedy, the
passing direction oscillates in ambiguous configurations, or the robot
freezes. **COMPASS** treats the *temporal consistency* of the passing decision
as a first-class design objective: the passing relationship is a topological
class bound to a person's track ID, switching is governed by a single **leaky
evidence accumulation** rule that unifies margin, dwell, and point-of-no-return,
and safety is layered **lexicographically** on top so consistency never
overrides collision avoidance. Five properties (P1–P5: anti-oscillation,
switching-rate bound, safety dominance, maneuver completion, deadlock escape)
are stated formally and exercised by property tests.

**Tested on:** ROS 2 **Jazzy** + Nav2 (custom `nav2_core::Controller` plugin)
+ Gazebo **Harmonic** (`gz sim` 8.10).

## 📢 News

- **2026-07** — arXiv-ready English draft (33 pp, [`paper/arxiv/`](paper/arxiv/))
  with measured offline evaluation (R1 ablation · R3 latency · R5 ρ-sweep),
  reproducible via [`compass_eval`](src/compass_eval/) and guarded by
  [`scripts/check_paper_numbers.py`](scripts/check_paper_numbers.py).

## Overview

| Component | What it is |
|---|---|
| [`src/compass_core`](src/compass_core/) | ROS-independent decision core (C++): class lifecycle, leaky-evidence commitment, lexicographic safety; P1–P5 property tests |
| [`src/compass_nav2`](src/compass_nav2/) | `nav2_core::Controller` plugin wrapping the core + path-following cruise |
| [`src/compass_msgs`](src/compass_msgs/) | `People`/`Person` messages (`/people` input) |
| [`src/compass_eval`](src/compass_eval/) | Offline experiment harness driving the actual decision core; results under [`results/`](src/compass_eval/results/) |
| [`sim/`](sim/) | `compass_sim` — minimal Gazebo Harmonic testbed (office world, diff-drive robot, reactive pedestrian, Nav2 bringup) |

**Key ideas**
- **Track-ID-bound topological class** — the left/right passing relationship
  survives across cycles through an explicit lifecycle rule (spawn, removal,
  merge, split, TTL), stabilizing cross-cycle correspondence.
- **One switching equation** — challenger advantage integrates into a leaky
  accumulator; switching fires only when accumulated evidence crosses a
  progress-hardened threshold, subsuming hysteresis, dwell timers, and
  point-of-no-return in a single rule with a provable switching-rate bound.
- **Lexicographic safety** — safety feasibility is evaluated before, and
  strictly dominates, the commitment machinery; a thrash guard bounds
  safety-induced replanning.

## 📄 Paper

> **COMPASS: Temporally Consistent Topological Passing Decisions for Socially
> Aware Robot Navigation** — Jungmo Kang.
> [English PDF](paper/arxiv/main.pdf) (33 pp, arXiv-ready) ·
> [LaTeX source](paper/arxiv/main.tex) ·
> [Korean draft](paper/paper_draft.md)

If COMPASS is useful in your research, please cite it
(see also [`CITATION.cff`](CITATION.cff)):

```bibtex
@unpublished{kang2026compass,
  author = {Kang, Jungmo},
  title  = {{COMPASS}: Temporally Consistent Topological Passing Decisions
            for Socially Aware Robot Navigation},
  year   = {2026},
  note   = {Draft manuscript, available at
            \url{https://github.com/kjungmo/compass} under paper/arxiv/},
}
```

## 🔨 Build & test

Requires ROS 2 Jazzy (and Gazebo Harmonic for the testbed). From the
repository root:

```bash
# from the repository root, in a sourced ROS 2 Jazzy environment
colcon build && source install/setup.bash
colcon test && colcon test-result --verbose

# Reproducing the experiments (ablation, latency, rho-sweep)
./build/compass_eval/compass_eval ablation src/compass_eval/results
./build/compass_eval/compass_eval latency
./build/compass_eval/compass_eval rho_sweep
```

## 📊 Measured results

Scope, stated plainly: these are **offline measurements of the actual decision
core** (`compass_eval` drives the same C++ code the Nav2 plugin runs; 5
scenarios × 50 seeds), **not** physical-simulation performance. Success rate,
collisions, social distance, and external baselines are deliberately left as a
pre-registered protocol in the paper's *planned evaluation* (§5.6). Every
number below is quoted from the committed measurement records under
[`src/compass_eval/results/`](src/compass_eval/results/), and
[`scripts/check_paper_numbers.py`](scripts/check_paper_numbers.py) keeps the
paper's citations of them honest.

<div align="center">
<img src="paper/figures/fig_oscillation_compare.png" width="620" alt="Ablation, passing-class oscillation count: full method commits with at most one switch per encounter while removing the accumulator or class correspondence produces ~30-37 switches on average (log scale)"/>
</div>

- **R1 — oscillation control:** removing the leaky accumulator (immediate
  argmin) yields **~98 switches per encounter** under ambiguous near-ties;
  the full method commits with **≤ 1**.
- **R3 — real-time headroom:** per-cycle worst-case latency at K=3 (8 classes,
  no pruning) is **p99 58.6 µs / max 651.3 µs** — about **1.30 %** of a 20 Hz
  (50 ms) control budget, decision core only.
- **R5 — freezing threshold:** the progress-hardening drive rate exposes a
  freezing onset at **v_lat ∈ (0.20, 0.35) m/s**, motivating the planned
  lateral-velocity separation.
- **Hypothesis correction (negative result kept):** removing hysteresis or
  progress hardening alone causes no regression in this regime — the primary
  anti-oscillation mechanism is the accumulator itself.

## 🤖 Gazebo testbed

[`sim/`](sim/) brings up a 10 × 8 m office world with a diff-drive robot
(2D lidar + IMU), a reactive pedestrian that crosses the robot's path, and a
Nav2 `controller_server` loading `compass_nav2::CompassController`:

```bash
ros2 launch compass_sim sim_bringup.launch.py headless:=true
```

See [`sim/README.md`](sim/README.md) for launch arguments, headless/WSL2
notes, and the smoke-test scope (build + bringup validation; quantitative
physical metrics are future work, per §5.6 of the paper).

## 📚 Documentation

| Document | Contents |
|---|---|
| [`paper/arxiv/main.pdf`](paper/arxiv/main.pdf) | The paper (EN, 33 pp): formalization, P1–P5, measured evaluation, planned-evaluation protocol |
| [`paper/paper_draft.md`](paper/paper_draft.md) | Korean draft of the same paper |
| [`src/compass_eval/results/`](src/compass_eval/results/) | Committed measurement records (R1/R3/R5) + raw CSV |
| [`sim/README.md`](sim/README.md) | Testbed usage, launch args, WSL2/headless guidance |
| [`src/README.md`](src/README.md) | Package-level notes |

## 🧭 Roadmap

Measured today: decision-core behavior (R1/R3/R5). Planned, pre-registered in
the paper (§5.6):

- [ ] Physical performance battery in Gazebo (success rate, collisions,
      minimum social distance, lateral jerk) across seeded pedestrian
      scenarios
- [ ] External baselines (R2) under an independent, symmetric tuning protocol
- [ ] Lateral-velocity separation (replace the core's `v_lat = |v_cmd|`
      approximation; relaxes the R5 freezing threshold)
- [ ] User study (R4) and a real-robot demo with AR trajectory overlay

## 🙏 Acknowledgements

COMPASS builds on the [Nav2](https://github.com/ros-navigation/navigation2)
controller-plugin architecture and [Gazebo](https://gazebosim.org/) Harmonic,
and positions itself against the topological / homotopy-aware social
navigation line of work (winding-number invariance, per-class parallel
optimization, social momentum — see the paper's Related Work, §2, for the
full lineage and citations).

## 💛 Sponsor

If COMPASS saves you time, consider
[sponsoring](https://github.com/sponsors/kjungmo). Sponsorship funds
maintenance, new features, and faster issue response.

## License

Apache-2.0.
