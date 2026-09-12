# COMPASS completion checkpoint

Status: repository-wide main merge preparation; repaired candidate awaiting
final independent AI review and current-head CI. The earlier internal-completion
conclusion at `7f5ce825`/`1f17ba42` is superseded. Those green runs did not cover
the manuscript drift, missing consistency checks or safety defects subsequently
identified. Do not use their success as approval of this candidate.

## Integration scope

PR #15 (`review/completion-gates`) now targets `main` and contains the #1 -> #2 ->
#14 stack. The #12/#13 observer/theorem changes have been integrated with further
corrections, not simply copied byte-for-byte. Other branch heads are preserved.
See [MERGE_READINESS.md](MERGE_READINESS.md) for the integration procedure and
issue-by-issue research boundaries. Main itself has not been merged or deployed;
no Omni email is to be sent by this preparation.

The goal is a reviewable **research repository** with consistent source,
configuration, evaluation outputs, paper, figures, metadata and CI. It is not
production certification, paper acceptance, or a claim that no future software
change will be necessary. Code packages are versioned 0.2.0 because public C++
layouts/interfaces changed; all consumers must rebuild.

## Implemented software and manuscript work

- Decision traces distinguish labels, transition eligibility and evidence.
  The opportunity scorer excludes events not confirmed before termination;
  the physical scorer has explicit input/measurement contracts.
- The follow-up observer uses log odds and requires 0.30 s of observed hold.
  The current 1,500-row CSV and manuscript are regenerated consistently:
  argmin has 72/250 censored trials and the synthetic label-flicker comparator
  71/250. Original saturation-only results remain available in pinned Git
  history, not silently claimed byte-identical to the corrected observer.
  Trace-on/trace-off equality concerns two executions of the current core.
- Measured progress consumes stamped pose displacement along a frozen plan
  normal, not cmd_vel. Repeated identical plans retain the epoch; geometry,
  class and safety/HOLD recommit changes start a new epoch. Both measured and
  proxy cumulative distance reset on genuine safety/HOLD class changes.
  Invalid measurement and pose/plan frame mismatches return zero commands.
- The opt-in candidate path evaluates, collision-checks and executes the same
  bounded unicycle rollout. Static clearance includes conservative intersample
  sweep padding. Unknown/out-of-map/missing costmaps fail closed. Braking or
  speed-limit changes regenerate and revalidate the selected rollout.
  Default legacy-ray TTC approach/recession and left/right conventions are
  corrected too; a blocked/unobserved legacy ray emits a complete zero twist.
- Explicit safety velocity limits preserve zero and small caps. `a_brake` is
  acceleration in m/s² and decrements the target speed by `a_brake * dt`.
  Invalid accumulator domains are rejected; invalid decision inputs cannot
  escape the Nav2 command adapter as an unhandled command computation error.
- P5 counts at most one intervention per decision time in `(t-W,t]`, enters
  absorbing zero-command HOLD at threshold, and requires explicit release.
  No unconditional escape/eventual-progress theorem is claimed.
- P1/P2/P4 distinguish pre-test evidence from post-reset storage, minimum
  crossing bounds from conditional response bounds, and finite clipping from
  asymptotic equilibrium equality. Synthetic comparator and Gaussian-driver
  limitations are stated. Physical freezing is not inferred from label blocking.
- Canonical LaTeX/PDF, the English entry point, Korean noncanonical companion,
  README/package/simulation documentation and generated figures are reconciled.
  Numeric, artifact-hash, local-link and package-version checks are in CI;
  manuscript compilation is a separate required job. ROS CI builds all five
  packages; simulation asset/syntax verification is not a Gazebo run.

Initial investigation and remediation records are in
[reviews/merge-code.md](reviews/merge-code.md),
[reviews/merge-paper.md](reviews/merge-paper.md),
[reviews/merge-docs.md](reviews/merge-docs.md) and
[reviews/merge-consistency.md](reviews/merge-consistency.md).
They are AI investigations, not human approvals or experimental evidence.

## Verification gate

The final candidate must pass all of:

```bash
bash scripts/test_compass_observability.sh
bash scripts/test_completion_gates.sh
bash scripts/test_compass_response.sh
python3 scripts/check_observer_results.py
python3 scripts/check_switch_bounds.py
python3 scripts/check_paper_numbers.py
python3 scripts/check_repo_consistency.py
python3 -m unittest discover -s scripts -p test_repo_consistency.py
bash scripts/build_paper.sh --check
```

These cover 50,000-cycle state equivalence, 1,500 current CSV rows, progress/
candidate/safety contracts, 16 opportunity and 8 physical-scorer tests,
response-profile and theorem-bound diagnostics, repository consistency, and
manuscript compilation. They do not measure physical performance. ROS-specific
gtests require actual ROS Jazzy CI: local dependency stand-ins used during
development are explicitly not ROS validation. A fresh exact-candidate AI review
and current-head green CI remain required before the integration PR is ready.

The deterministic unicycle diagnostic remains scoped: `legacy_proxy` times out
at 60 s, while both measured profiles reach the declared goal at 17.45 s. These
identical measured results do not isolate a responsive-profile benefit. The
fixture uses scripted costs and is neither full Nav2/Gazebo nor hardware.

## Explicit external gates

- The 480-case physical protocol still needs bound geometry, actor traces,
  independently annotated opportunity/free-motion oracle and logging adapter.
- Gazebo/robot execution, physical freezing, goal success, collision/social
  distance outcomes, external baselines and human motion-legibility studies
  remain unmeasured. The local environment has no ROS/Gazebo/container runtime;
  ROS build/testing is supplied by CI, not substituted physical evidence.
- The frozen-normal progress estimator is a coherent single-maneuver proxy;
  it is not a general multi-person/Frenet ground truth. Mixed equally split
  side classes hold. Real replanning behavior needs integration evaluation.
- Archived timing is not a latency/WCET measurement of the new paths.
  Independent human review and actual main-merge/deployment decisions remain
  separate from AI/software verification.

Do not repeatedly build or invent optional extensions to fill these external
research gaps. Do not send email, merge to main, deploy, or delete branches as
part of this preparation. At handoff, record the final candidate and CI links;
any subsequent code change invalidates that exact-candidate readiness decision.
