# Candidate trajectory integration checkpoint

This is software groundwork, not end-to-end Nav2 or physical validation.

## Implemented contract

`IEnvQuery::candidate_trajectory(c)` optionally supplies the candidate evaluated
by `CostEvaluator::J`. The default is nullopt, retaining the legacy cost path.
Supplying an empty or invalid trajectory makes that candidate unavailable; there
is no silent fallback. All ABI consumers must rebuild for source version 0.2.0
after virtual-method and public-layout changes. The old raw-term helpers
intentionally still describe legacy costs; this is not a published release.

Predictions start at the current input pose and run for exactly one second.
Time must increase with intervals no greater than 0.1 s. Each interval follows
exact unicycle integration of its starting sample's linear/angular command.
This rules out cost reductions from shortening the horizon or teleporting to a
better endpoint. These checks establish internal consistency, not collision safety.

Costs use endpoint distance, trapezoidal social-density integration with constant
heading/velocity person predictions, and the integral of squared angular command.
Normalization and rule weights remain unchanged and need calibration before new
performance claims. Nonfinite geometry, invalid timing, command/pose mismatch and
invalid person covariance are rejected on this path.

`candidateRollout` predicts the existing path-tracking command at 20 Hz for one
second, at a supplied constant nonnegative speed. L is positive path-normal bias,
R negative. Empty classes have zero bias. Mixed-side classes and degenerate paths
are rejected. This does not establish that a path passes each person on its
assigned side, or provide a general multi-person homotopy planner.

## Verification

The standalone candidate contract test checks mirrored trajectories, separate
goal/social/effort ranking with rule weight disabled, moving-person prediction,
invalid trajectories, unsupported classes and legacy fallback. Both completion
gates and ROS CMake register it. The 50,000-cycle trace/no-trace comparison uses
the same current core on both sides. The 1,500-row canonical CSV regression uses
the corrected observer; its observer fields are not byte-identical to the original
`9fe495a` archive. Label metrics are unchanged in that battery, not every old
execution path. Safety fixes are separate from the candidate opt-in.

## Nav2 opt-in integration

`use_candidate_trajectories=false` selects the legacy cost/command path. It does
not bypass integrated safety repairs or imply historical binary equivalence.
It is independent of `use_measured_progress` and `k_rho=0.5`; configure each
explicitly. When enabled,
`CostmapEnvQuery` supplies the generated trajectory and checks the circular robot
proxy along it. Unknown cells, map boundaries and a missing costmap fail closed.
The swept static-clearance lower bound includes an inter-sample half-arc-length
padding; checking only discrete rollout poses would miss some between-tick hits.
In every controller mode, pose and plan headers must match the costmap's
global frame; mismatches produce a zero command rather than an implicit coordinate
reinterpretation. Automatic frame transformation is not implemented here.
Person TTC uses the same rollout and constant-velocity predictions. The adapter
builds candidates after goal taper and external speed caps. A safety-reduced
speed is regenerated and revalidated; the first command of that exact rollout is
then emitted. Slower motion is not presumed safe for dynamic encounters.

The ROS contract tests cover obstacle-side discrimination, unknown cells, map boundaries,
missing maps, approaching/receding people, changed-speed revalidation and emitted
sample correspondence. Final-source ROS CI must pass before this software gate
is closed; earlier successful runs do not certify later edits.
The circular footprint and 0.30 m person radius are declared evaluation proxies,
not platform-calibrated geometry. The braking parameter `a_brake` is in m/s²
and the per-cycle speed decrement is `a_brake * dt`; output command limits are
not a guarantee that hardware realizes that deceleration.

The default-mode legacy environment is also corrected: approaching TTC has the
proper sign, L/R ray offsets agree with emitted steering, missing/unobserved map
queries are unavailable, and an immediately blocked/unobserved legacy ray results
in a complete zero twist. These intentional safety corrections retain the default
knob values, not all historical runtime behavior. The legacy ray/TTC estimates
are still proxies, not equivalent to the candidate swept-trajectory contract.

Only after CI should the new path be evaluated end to end. Existing
scripted-cost closed-loop results do not become evidence for it retroactively.
Gazebo/robot freezing, goal success, warranted-switch responsiveness and human
motion-legibility measurements remain separate external validation gates.
