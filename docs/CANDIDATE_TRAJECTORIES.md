# Candidate trajectory integration checkpoint

This is software groundwork, not end-to-end Nav2 or physical validation.

## Implemented contract

`IEnvQuery::candidate_trajectory(c)` optionally supplies the candidate evaluated
by `CostEvaluator::J`. The default is nullopt, retaining the archived cost path.
Supplying an empty or invalid trajectory makes that candidate unavailable; there
is no silent fallback. All ABI consumers must rebuild after the virtual method
addition. The old raw-term helpers intentionally still describe archived costs.

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
gates and ROS CMake register it. Existing 50,000-cycle state equivalence and
1,500-row archived CSV regression must remain exact.

## Remaining software gate

1. Add an explicit opt-in Nav2 mode whose environment caches these trajectories
for the cycle. Evaluate swept robot clearance and predicted person encounters on
the same trajectories; unknown/out-of-map geometry must fail closed. The existing
offset-ray clearance and class-independent TTC are not sufficient for this mode.
2. Execute the evaluated candidate command. Account for goal taper, speed limits,
and safety braking before final acceptance; a modified command needs validation
on its own rollout. Do not assume that slowing a dynamic encounter is safer.
3. Test obstacle-side discrimination, unknown cells, approaching/receding people,
speed-limit/braking consistency and selected-command correspondence in ROS CI.
Keep the legacy default and document unsupported mixed-side maneuvers explicitly.

Only after these gates should the new path be evaluated end to end. Existing
scripted-cost closed-loop results do not become evidence for it retroactively.
Gazebo/robot freezing, goal success, warranted-switch responsiveness and human
motion-legibility measurements remain separate external validation gates.
