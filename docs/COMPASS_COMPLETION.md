# COMPASS completion checkpoint

Status: candidate trajectory cost seam verified by ROS CI. Opt-in Nav2 swept
environment and selected-command integration are implemented locally and await
their own CI. This is the checkpoint for authorized
continuation; do not resend either Omni email. Do not merge/deploy to main.

## Remote stack and scope

- PR #2: review/observability-metrics, original remote head 27fd2519.
- PR #14: review/responsive-progress, original remote head 41e7837e.
- PR #15: review/completion-gates, based on PR #14.
- Other round5 observer/theorem branches are concurrent work; do not overwrite them.

The goal is a reviewable, tested software implementation with explicit research
limits. It is not a claim that no future software changes can ever be necessary.

## Completed in this branch

1. Counterexample fix: opportunities not confirmed before run termination are
excluded, including collisions before the opportunity exists. 16 opportunity tests pass.
2. ROS-independent frozen path-normal displacement estimator, wired into
CompassController behind use_measured_progress=false. It consumes stamped pose
displacement, not cmd_vel. Class/frame changes anchor a new interval. Reverse
motion is signed. Repeated/reversed time, stale gaps, pose jumps and absent path
fail explicitly. ROS returns a zero command on invalid measurements.
3. ROS parameters: progress_max_gap=.25 s, progress_max_speed=2 m/s,
progress_length=1 m. These are proposed configured limits, not calibrated values.
setPlan preserves the measured epoch for identical XY geometry/frame despite new
stamps. Changed geometry still resets progress; genuine frequent replanning
therefore limits hardening and needs integration evaluation.
Mixed equally split side classes are unsupported and cause a hold; no multi-person
maneuver-progress estimator is claimed. A fixed plan-normal is a proxy for a
single coherent maneuver, not a general ground-truth lateral-progress solution.
4. Executable closed-loop unicycle diagnostic plus scorer. Uses actual
DecisionCore::step_evals and pathTrackingAngularZ with a single endpoint, but
SCRIPTED costs and two candidate endpoints. Not the complete Nav2 controller,
Gazebo, hardware dynamics, or an independent physical oracle. No stochastic seeds
or repeated independent trials are claimed for this deterministic diagnostic.
5. CI for standalone regression and ROS Jazzy colcon build/test.
6. SafetyResult/DecisionOutput carry an explicit safety velocity-limit flag.
The Nav2 command adapter honors zero/small braking bounds instead of treating
these as normal measured-speed startup values. Default core decision behavior
and archived CSV are unchanged; consumer binaries must rebuild for output layout.
STOP/HOLD remain zero. The existing acceleration decrement semantics are unchanged.
A ROS regression tests identical-path republish versus geometry/frame changes.
7. An opt-in candidate-trajectory Nav2 path now evaluates and executes the same
one-second unicycle rollout. Static clearance sweeps a circular robot proxy over
the costmap; unknown, out-of-map, absent-map and invalid trajectories fail closed.
TTC uses constant-velocity people predictions and distinguishes approaching from
receding motion. Goal taper, Nav2 speed limits and safety braking are applied in
the declared order. If braking changes speed, the selected rollout is regenerated
and revalidated before its first command is emitted. The default remains false.

## Local evidence

Candidate trajectory cost seam at 30b6e7819167b287112c8e3afa20c71e018494c4
passed standalone and ROS Jazzy CI:
https://github.com/kjungmo/compass/actions/runs/34703521524
Completion gates pass, including candidate ranking
and malformed-rollout rejection. The 50,000-cycle state-equivalence check and
all 1,500 canonical CSV rows remain exact; 16 opportunity and 8 physical-scorer
tests pass. These are local software checks, not new physical results.

CI at remote commit af55731892eb099124e0c272dd3e12ced618edbe passed:
core/messages/evaluator/controller build succeeded; colcon reports 50 tests,
0 errors, 0 failures, 0 skipped. Standalone job also passed.
Evidence: https://github.com/kjungmo/compass/actions/runs/34701856351
This validates the PIC correction after run 34698467661 failed at shared linking.
Safety-velocity and identical-plan changes at bb0e8293c1b949e398df9f4efb0df5748898cfd2
passed both standalone and ROS Jazzy jobs:
https://github.com/kjungmo/compass/actions/runs/34702249850
The subsequent Nav2 environment/command integration needs a new CI run.
Local safety contract tests cover zero/small bounds, normal startup, STOP/HOLD,
and nonfinite limits. The 50,000-cycle/1,500-row regression remains green with
16 opportunity tests after these changes. The plan-republish gtest is now also covered by that successful ROS CI.

Run bash scripts/test_completion_gates.sh. All estimator checks and 16 opportunity
plus 8 physical-metric tests pass. Previous 50,000-cycle/1,500-row legacy regression
and 3,750-run offline evidence remain scoped as described in RESPONSIVENESS.md.

Diagnostic: legacy_proxy timeout at 60 s; legacy_measured and
responsive_measured reach the declared goal at 17.45 s. Both measured results
are identical: this fixture does not isolate a benefit of the responsive knobs.
Results and trace hashes are in src/compass_eval/results/closed_loop_diagnostic.
Re-running emits the underlying motion JSONL. No physical/Gazebo claim is closed.

## Review findings and gates

- PR #2: request changes until the unconfirmed-opportunity correction is included.
- PR #14: conditional research-only acceptance after CI; not a production-ready
response guarantee. Intermittent switches rise .4 -> .6, mid-reversal stays zero,
and a warranted oracle has not validated either scenario.
- Candidate integration: the optional Nav2 path now supplies class-specific
rollouts to CostEvaluator, checks swept static clearance and predicted TTC, and
emits the revalidated selected rollout command. Local standalone gates pass;
ROS CMake/controller tests for this change await CI. This bounded implementation
still does not solve mixed-side multi-person homotopy or validate physical
performance. See CANDIDATE_TRAJECTORIES.md for its explicit limitations.
- The small/zero safety-bound bootstrap defect is fixed in code with a regression;
its ROS build and plan-republish regression passed at bb0e8293.
- ROS build/test passed through candidate costs at 30b6e781; latest Nav2
environment/command changes need their own CI.
No Gazebo test executed. Local ROS/container runtime is still absent.
- 480 physical cases remain pending: bind geometry, actor traces, independent
opportunity/free-motion oracle and controller adapter before physical experiments.
- Robot deployment and main merge require a separate concrete decision.

## Continuation protocol

Check the completion PR CI once per scheduled run. If still running, wait.
If failed, inspect logs and fix the concrete defect, then rerun only relevant gates.
Preserve remote parent/head changes. Update this file with exact tested commits.
Do not declare whole-system completion solely because CI passes. The latest user
clarification keeps candidate-specific costs and control integration in software
scope. After this CI, continue bounded candidate-trajectory/cost implementation
and tests; preserve the archived baseline via an explicit option. Distinguish
such internal code gaps from Gazebo/hardware measurement gates. Disable only
when authorized software work is complete or further work is demonstrably blocked;
record a precise remaining validation handoff and do not claim physical success. If platform limits prevent progress, record the blocker and stop
repeating identical attempts. No usage-quota/reset-time API is available.
