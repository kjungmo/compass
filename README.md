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
and safety decisions are layered **lexicographically** on top. Five conditional
properties (P1–P5: anti-oscillation, switching-rate bound, safety dominance,
maneuver completion, finite-window escalation to HOLD) are stated formally and
exercised by software tests. Safety priority is a decision contract, not a proof
of physical collision avoidance; HOLD is an absorbing zero-command state, not
a guarantee of eventual goal progress.

**Software validation:** standalone C++17 tests and ROS 2 **Jazzy** + Nav2
build/tests. CI builds all five packages and checks simulation install assets and
Python syntax; it does not launch Gazebo. The **Harmonic** testbed (`gz sim` 8) has historical smoke
notes; the current optional controller paths have not been validated in Gazebo
or on a robot. See [the implementation checkpoint](docs/COMPASS_COMPLETION.md).

## 📢 News

- **2026-09** — English manuscript and offline observer results corrected after
  review; optional measured-progress and candidate-rollout interfaces added.
  The current [English manuscript](paper/arxiv/main.tex) is a research draft,
  not a deployment certification. See [results and provenance](src/compass_eval/results/README.md).

## Overview

| Component | What it is |
|---|---|
| [`src/compass_core`](src/compass_core/) | ROS-independent decision core (C++): ID-indexed labels/state, leaky-evidence commitment and safety-priority contracts |
| [`src/compass_nav2`](src/compass_nav2/) | `nav2_core::Controller` plugin wrapping the core + path-following cruise |
| [`src/compass_msgs`](src/compass_msgs/) | `People`/`Person` messages (`/people` input) |
| [`src/compass_eval`](src/compass_eval/) | Offline experiment harness driving the actual decision core; results under [`results/`](src/compass_eval/results/) |
| [`sim/`](sim/) | `compass_sim` — minimal Gazebo Harmonic testbed (office world, diff-drive robot, reactive pedestrian, Nav2 bringup) |

**Key ideas**

- **Track-ID-bound passing label** — persistent IDs identify left/right
  commitments across cycles. The paper specifies a fuller lifecycle (spawn,
  removal, merge, split, TTL); the current core enumerates input-order top-K
  labels and does not implement that complete runtime lifecycle or verify
  trajectory winding classes.
- **One switching equation** — challenger advantage integrates into a leaky
  accumulator; switching fires only when accumulated evidence crosses a
  progress-hardened threshold, subsuming hysteresis, dwell timers, and
  point-of-no-return in a single rule with a provable switching-rate bound.
- **Lexicographic safety** — safety feasibility is evaluated before, and
  strictly dominates, the commitment machinery; a rolling-window thrash guard
  enters HOLD when enough interventions occur in that window. Sparse repeated
  interventions need not trigger HOLD.

## 📄 Paper

> **COMPASS: Temporally Consistent Topological Passing Decisions for Socially
> Aware Robot Navigation** — Jungmo Kang.
> [English PDF](paper/arxiv/main.pdf) (research draft) ·
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

The decision-core gates require a C++17 compiler and Python 3, not ROS:

```bash
bash scripts/test_compass_observability.sh
bash scripts/test_completion_gates.sh
bash scripts/test_compass_response.sh /tmp/compass_response_results
python3 scripts/check_observer_results.py
python3 scripts/check_switch_bounds.py
python3 scripts/check_paper_numbers.py
python3 scripts/check_repo_consistency.py
```

The [repository consistency gate](scripts/check_repo_consistency.py) checks named
result rows, documentation regressions and declared artifact hashes; it is not a
proof of every claim. With TeX Live (including science/extra packages), Poppler
and ripgrep installed, check the [canonical paper build](scripts/build_paper.sh):

```bash
bash scripts/build_paper.sh --check
```

This builds in a temporary directory without replacing the committed PDF.

For ROS packages, start in a sourced ROS 2 Jazzy environment with Nav2 and
the declared package dependencies installed. From the repository root:

```bash
# from the repository root, in a sourced ROS 2 Jazzy environment
colcon build --base-paths src --packages-up-to compass_nav2 compass_eval --cmake-args -DBUILD_TESTING=ON
source install/setup.bash
colcon test --packages-select compass_core compass_nav2 compass_eval
colcon test-result --verbose

# Reproducing the experiments (ablation, latency, rho-sweep)
mkdir -p /tmp/compass_eval_results
./build/compass_eval/compass_eval ablation /tmp/compass_eval_results
./build/compass_eval/compass_eval latency
./build/compass_eval/compass_eval rho_sweep
```

The ablation command writes CSV to the supplied directory and prints its table;
latency and rho-sweep print tables. Preserve the committed measurements when
running new experiments. Exact seeded regression currently targets Linux
g++ 13.3/libstdc++ (the recorded CI toolchain);
latency depends on hardware, compiler, build flags and system load. Rebuild all
dependent binaries for the 0.2.0 source version: public object layouts and the
environment query interface changed. This version is not a published release.
See [package notes](src/README.md) and [sim build](sim/README.md).

## 📊 Measured results

Scope, stated plainly: these are **offline measurements of the actual decision
core** (`compass_eval` drives the same C++ code the Nav2 plugin runs; 5
scenarios × 50 seeds), **not** physical-simulation performance. Success rate,
collisions, social distance, and external baselines are deliberately left as a
pre-registered protocol in the paper's *planned evaluation* (§5.6). Every
number below is quoted from the committed measurement records under
[`src/compass_eval/results/`](src/compass_eval/results/). The observer was corrected
using log-odds and 0.30 s of observed follow-up: current CSV observer fields are
not byte-identical to the original `9fe495a` archive. Decision-label metrics in
that battery are unchanged. Numerical checks cover the CSV/table correspondence;
they do not validate every prose claim or certify physical performance.
Default knob values and both opt-in flags are preserved; runtime safety-defect
corrections intentionally change Nav2 behavior, including legacy TTC/occupancy,
frame checks and time-scaled braking. This is not a binary or behavioral rollback
to the original archive.

<div align="center">
<img src="paper/figures/fig_oscillation_compare.png" width="620" alt="Offline passing-label switch counts: full method has at most one switch per encounter; immediate argmin and a synthetic flicker stress comparator average about 30 and 37 switches across five scenarios (log scale)"/>
</div>

- **R1 — oscillation control:** removing the leaky accumulator (immediate
  argmin) yields **~98 switches per encounter** under ambiguous near-ties;
  the full method commits with **≤ 1**.
- **R3 — archived latency:** the original default-cost decision core at K=3
  (8 classes, no pruning) measured **p99 58.6 µs / max 651.3 µs** — about
  **1.30 %** of a 20 Hz (50 ms) budget for that recorded maximum. This is not
  a worst-case execution-time guarantee or timing evidence for the new candidate
  path or full ROS pipeline.
- **R5 — offline transition blocking:** the scripted progress-rate input exposes
  a switch-blocking interval at **v_lat ∈ (0.20, 0.35) m/s**. It does not measure
  a stopped robot, physical freezing, collision rate or goal failure.
- **Corrected decision-stream observer:** argmin and the synthetic
  no-correspondence comparator are censored in **72/250** and **71/250** runs,
  respectively. This common label-stream scoring convention is not evidence of
  human or motion legibility.
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
notes, and the historical smoke-test scope. Re-run bringup for the current source
before relying on it; quantitative physical metrics are future work, per §5.6.

## 📚 Documentation

| Document | Contents |
|---|---|
| [`paper/arxiv/main.pdf`](paper/arxiv/main.pdf) | English research draft: conditional P1–P5, offline evaluation, planned physical protocol |
| [`paper/paper_draft.md`](paper/paper_draft.md) | Korean draft of the same paper |
| [`src/compass_eval/results/`](src/compass_eval/results/) | Corrected R1/R5, archived R3, raw CSV and scoped research diagnostics |
| [`sim/README.md`](sim/README.md) | Testbed usage, launch args, WSL2/headless guidance |
| [`src/README.md`](src/README.md) | Package-level notes |
| [`src/compass_eval/RESPONSIVENESS.md`](src/compass_eval/RESPONSIVENESS.md) | Opt-in responsiveness profile, measured-progress contract and pending physical pilot |
| [`docs/CANDIDATE_TRAJECTORIES.md`](docs/CANDIDATE_TRAJECTORIES.md) | Opt-in candidate rollout, environment and command contract; limitations |

## 🧭 Roadmap

Measured today: offline decision-core behavior, archived latency, and a
scripted-cost unicycle diagnostic. Measured progress and candidate trajectories
are software opt-ins; they are not validated physical results. Remaining work:

- [ ] Physical performance battery in Gazebo (success rate, collisions,
      minimum social distance, lateral jerk) across seeded pedestrian
      scenarios
- [ ] External baselines (R2) under an independent, symmetric tuning protocol
- [ ] Calibrate and physically validate the implemented measured-progress and
      candidate-rollout opt-ins, including mixed-side/multi-person limitations
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
