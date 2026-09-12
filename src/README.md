# COMPASS source packages

The implementation is present. The source package version is 0.2.0; this marks
interface/layout changes, not a published or robot-qualified release.

| Package | Responsibility |
|---|---|
| [`compass_core`](compass_core/) | ROS-independent C++17 decision logic, class state, accumulator, cost and safety contracts |
| [`compass_msgs`](compass_msgs/) | ROS `Person`/`People` messages consumed on `/people` |
| [`compass_nav2`](compass_nav2/) | Nav2 controller adapter, path tracking, optional measured progress and candidate rollout execution |
| [`compass_eval`](compass_eval/) | Offline decision harness, trace executable and contract tests |
| [`../sim`](../sim/) | Separately located `compass_sim` launch/configuration/assets and smoke-test helpers |

Build commands and standalone gates are in the [root README](../README.md).
Use dependency-aware `--packages-up-to` selection on a fresh ROS workspace;
`compass_nav2` requires the local `compass_msgs` package as well as the core.
Core source is ROS-independent, but its package CMake uses `ament_cmake`;
the standalone scripts compile it directly without ROS.
The paper's complete TTL/merge/split and winding-sign lifecycle is an intended
design contract. Current enumeration uses input-order top-K ID labels; the
integrated adapter does not establish full lifecycle or homotopy realization.

## Defaults and opt-ins

Default `Knobs{}` retains `k_rho=1.0`; both `use_measured_progress` and
`use_candidate_trajectories` default to false. The research responsiveness profile
uses `k_rho=0.5`; enabling measured progress does not select that profile. The Nav2
wrapper reads `FollowPath.k_rho` from its parameter file when configured.

Measured progress projects stamped realized pose displacement onto a frozen
path normal. Invalid intervals fail closed; it is not a general multi-person
maneuver estimator. Candidate rollouts evaluate and emit a bounded one-second
unicycle trajectory, with a circular-footprint costmap proxy and simple person
predictions. See [responsiveness](compass_eval/RESPONSIVENESS.md) and
[candidate trajectories](../docs/CANDIDATE_TRAJECTORIES.md) for exact contracts.

All dependent binaries must be rebuilt together: public input/output/state
layouts and the `IEnvQuery` virtual interface changed. Retained entrypoint
signatures are not an ABI-compatibility promise.

## Evidence boundary

The 1,500-row canonical regression refers to the current, corrected observer CSV.
The original `9fe495a` archive remains in Git; observer fields changed, while its
switch counts, sign-change rates and entropy are retained by the tested battery.
Safety-window fixes and optional paths are not a claim that all old execution
behavior is unchanged. The 50,000-cycle trace test compares instrumented and
uninstrumented calls to the same current core, not the current core to old binaries.

Runtime safety corrections also apply with both opt-ins disabled: legacy TTC
uses the approaching relative-velocity sign, L/R ray offsets match command bias,
missing maps/unknown/out-of-map queries are unavailable, and pose/plan headers
must match the costmap frame. An immediately blocked/unobserved legacy ray yields
a zero twist. These repairs do not turn the legacy ray/TTC estimates into a
swept-trajectory collision guarantee. Braking is `a_brake * dt` with `a_brake` in
m/s². A genuine safety switch or HOLD-entry recommit clears cumulative progress
for both legacy-proxy and measured modes. Declared accumulator-domain checks
reject invalid knobs, and nonpositive
or nonfinite decision periods are rejected. The adapter catches decision
`invalid_argument` errors to return a zero command; this is not exhaustive
validation of every possible perception/configuration fault.

ROS CI build/tests are software integration evidence. Historical Gazebo smoke
observations and deterministic scripted-cost diagnostics do not validate current
end-to-end performance. Physical safety, freezing, goal success, independent
warranted-switch responsiveness and human motion legibility remain unmeasured.
