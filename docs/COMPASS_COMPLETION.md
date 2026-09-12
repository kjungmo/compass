# COMPASS completion checkpoint

Status: awaiting ROS CI and final review. This is the checkpoint for authorized
continuation; do not resend either Omni email. Do not merge/deploy to main.

## Remote stack and scope

- PR #2: review/observability-metrics, original remote head 27fd2519.
- PR #14: review/responsive-progress, original remote head 41e7837e.
- Completion branch: review/completion-gates, based on PR #14.
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
setPlan resets the measured epoch and cumulative progress; frequent replanning
therefore limits accumulated hardening and must be evaluated in integration.
Mixed equally split side classes are unsupported and cause a hold; no multi-person
maneuver-progress estimator is claimed. A fixed plan-normal is a proxy for a
single coherent maneuver, not a general ground-truth lateral-progress solution.
4. Executable closed-loop unicycle diagnostic plus scorer. Uses actual
DecisionCore::step_evals and pathTrackingAngularZ with a single endpoint, but
SCRIPTED costs and two candidate endpoints. Not the complete Nav2 controller,
Gazebo, hardware dynamics, or an independent physical oracle. No stochastic seeds
or repeated independent trials are claimed for this deterministic diagnostic.
5. CI for standalone regression and ROS Jazzy colcon build/test.

## Local evidence

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
- End-to-end blocker: CostEvaluator goal/social/effort terms ignore candidate
class; only the rule preference differs, aside from availability. Therefore the
scripted cost response evidence is not validation of real Nav2 candidate ranking.
A class-specific trajectory generator and cost evaluation are further algorithm
work, not something to paper over by changing numerical knobs.
- The wrapper's existing speed bootstrap can ignore a small/zero safety velocity
limit outside STOP/HOLD. Review the safety command contract before robot use.
- ROS build/test is pending; /opt/ros, colcon and container runtime are absent
locally. Use CI output as evidence, not a guessed pass. No Gazebo test executed.
- 480 physical cases remain pending: bind geometry, actor traces, independent
opportunity/free-motion oracle and controller adapter before physical experiments.
- Robot deployment and main merge require a separate concrete decision.

## Continuation protocol

Check the completion PR CI once per scheduled run. If still running, wait.
If failed, inspect logs and fix the concrete defect, then rerun only relevant gates.
Preserve remote parent/head changes. Update this file with exact tested commits.
When software CI passes and reviewable PRs clearly expose the above external
research/robot gates, report that scoped completion and disable the continuation
automation. If platform limits prevent progress, record the blocker and stop
repeating identical attempts. No usage-quota/reset-time API is available.
