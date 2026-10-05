# compass_nav2 — deployment notes

`compass_nav2::CompassController` is the Nav2 controller plugin around the
`compass_core` decision layer. This page covers what an operator needs when
running it under `controller_server`. It does not change the research scope
stated in the [root README](../../README.md): the plugin is a research
implementation, not a qualified product.

Example configuration: [`config/compass_params.yaml`](config/compass_params.yaml).
Knob semantics are in the paper and in `compass_core/knobs.hpp`.

## Parameters added for deployment

All are under the plugin name (`FollowPath.` below). With these defaults the
commanded motion is the published behaviour, with one exception that is on by
default: on Humble only, a new plan arriving after the control loop has been
idle longer than `task_gap_reset_s` resets STOP/HOLD (see
[STOP and HOLD](#stop-and-hold-are-latched)). Everything else added here only
reports, or is an option that is off by default.

| Parameter | Default | Meaning |
|---|---|---|
| `people_topic` | `"/people"` | `compass_msgs/People` input. A relative name resolves in the controller_server namespace (one tracker per robot). |
| `people_timeout_s` | `0.5` | People input is *stale* when no message arrived since configure or the latest is older than this. Age = max(steady-clock time since receipt, ROS-clock age of a non-zero header stamp). |
| `people_stale_action` | `"warn"` | `warn`: log and report only; the decision still uses the last message regardless of age, as the paper (§4.5) states. `hold`: zero twist while stale; `hold` is an adapter option beyond the published method. |
| `diagnostics_period_s` | `1.0` | Period of the `/diagnostics` status (wall timer, while active); `0` disables it. |
| `task_gap_reset_s` | `0.5` | Humble only, see [STOP and HOLD](#stop-and-hold-are-latched); `0` disables. Ignored on Iron and later. |
| `stop_release_dwell_s`, `hold_release_after_s` | `-1` | Optional STOP/HOLD release, off by default and beyond the published method; see [Optional release](#optional-release-off-by-default-not-part-of-the-published-method). |

Why 0.5 s for `people_timeout_s`: it is a quarter of the default `ttc_min`
(2.0 s), and a person at 1.4 m/s closing head-on with the robot at 0.5 m/s
moves about 0.95 m in that time, more than the default `d_safe` (0.5 m).

### Tracker requirements for the freshness check

- Publish at a fixed rate, **including empty lists** when nobody is tracked. A
  tracker that publishes only when it sees people makes an empty corridor look
  stale (WARN, and a zero twist under `hold`).
- Keep the tracker's clock synchronised with the robot's ROS clock when it
  stamps messages: the age is the larger of the receipt age and the stamp age,
  so a skewed or replayed stamp reads as stale. An unset (zero) stamp is
  ignored and only the receipt age counts.

### Invalid configuration

Every knob is range-checked at configure. An out-of-range value throws
`std::invalid_argument` (the same type the core's accumulator-knob validation
throws) with a message such as
`FollowPath.d_safe = -0.5 is outside the allowed range [0, inf)`.
controller_server does not catch it, so bring-up aborts. Observed on Jazzy:
`Caught exception in callback for transition 10`, the message above, then
`Lifecycle node controller_server does not have error state implemented`;
controller_server is not returned to a usable state and its local costmap is
not cleaned up. Humble's
controller_server catches only pluginlib exceptions around plugin configure,
so the same path is expected there (CI checks only that configure fails with
the message). In both cases fix the YAML and restart controller_server.

## What the operator sees

- **Mode changes** are logged: `NORMAL -> STOP` (warning, with the committed
  class's TTC against `ttc_stop`), `NORMAL -> HOLD` (warning, with the
  intervention count in `W` against `n_thrash`), and the return to `NORMAL` on
  reset.
- **`/diagnostics`** carries one `DiagnosticStatus` named
  `"<node name>: compass (<plugin name>)"` (`hardware_id` = node namespace)
  with exactly these fields: `plugin`, `mode`, `committed_class`, `e_rev`,
  `rho`, `people_topic`, `people_count`, `people_age_s`, `people_timeout_s`,
  `people_stale`, `people_applied_last_cycle`, `people_tf_failures`,
  `people_tf_last_failure_age_s`, `seconds_since_compute`, `last_command`
  (the last command and why it is what it is), `stop_release_dwell_s` and
  `hold_release_after_s`. Level WARN while STOP or HOLD is
  active, while the people input is stale or missing, or while the latest
  people TF failure is within `people_timeout_s`; OK otherwise. It is published
  from a wall timer, so it keeps arriving when the control loop or `/clock`
  stops. `/diagnostics` is an absolute topic: with several robots in one ROS
  graph, remap it or run each robot in its own namespace and remap to it.
- **People TF failures** (people frame cannot be transformed into the costmap
  frame) are logged with the reason and counted; that cycle decides with no
  people, as before.
- All throttled warnings use a steady clock.

## STOP and HOLD are latched

As published, STOP and HOLD are absorbing: once entered, the plugin returns a
zero twist until the decision state is reset, even after the person has left.
Nav2's progress checker then aborts the goal. On Iron/Jazzy and later,
controller_server calls `reset()` when a FollowPath action ends, so the next
action starts in NORMAL.

Humble has no such hook. There, a new plan arriving after the control loop has
been idle longer than `task_gap_reset_s` resets STOP/HOLD (parity with
Jazzy's `reset()` at action end). A mid-task stall without a new plan does not
reset. Humble's controller_server calls `setPlan()` when a FollowPath action
starts and, for goal preemption, inside the running loop within one control
iteration of the previous call; only the first can follow an idle loop. The
default 0.5 s is ten periods at the default 20 Hz, so in-loop updates never
qualify. In the default Humble behaviour tree, a retry that restarts faster
than that (the FollowPath `ClearLocalCostmap` context recovery) keeps the
latched state until a later recovery (Spin, Wait, BackUp) opens a longer gap.
This differs from the paper's "HOLD retained until explicit external release"
only at task boundaries, as on Jazzy.

### Optional release (off by default, not part of the published method)

Two adapter parameters can end STOP or HOLD without a reset. They are not part
of the method the paper describes or evaluates; enabling them is a deployment
decision for the owner of the robot.

**`stop_release_dwell_s`** (default `-1`: STOP latches as published). When
`>= 0`, STOP returns to NORMAL only after all of the following have held in
every control cycle, continuously, for `stop_release_dwell_s` (steady clock);
any violation, and any cycle in which the check does not run (for example a
stale-input hold), restarts the dwell:

1. the people input is fresh (age `<= people_timeout_s`) and this cycle had no
   people TF failure, so the plugin is not blind;
2. the committed class is in the safe set **evaluated at the speed the robot
   would resume at** (the nominal speed: `cruise_speed` with the goal taper and
   speed limit; the candidate speed when candidate rollouts are on), not at its
   measured speed, which is zero while stopped: clearance `>= d_safe`, TTC at
   that speed `>= ttc_min` (not `ttc_stop`, for hysteresis), and feasible.

The releasing cycle still sends the zero twist; the next cycle decides in
NORMAL. Example: a person standing 0.7 m ahead gives a resume TTC of
0.7 / 0.45 = 1.56 s < `ttc_min` (2.0 s), so STOP never releases while they
stand there, although the measured-speed TTC of the halted robot is the 10 s
"no approach" value. The check uses the plugin's own estimates (the legacy
ray and line-of-sight TTC, or the candidate rollout); it is not a collision
guarantee.

**`hold_release_after_s`** (default `-1`: HOLD absorbing as published). When
set, it must be at least the thrash window `W`: a shorter value (including 0)
would release HOLD while the interventions that caused it are still being
counted, defeating the thrash guard. After HOLD has lasted that long,
`DecisionState::release_hold()` is called before the next decision, which
clears the intervention window. If the danger persists, the core enters HOLD
again after `n_thrash` new interventions, so the robot cycles HOLD → NORMAL →
HOLD about every `hold_release_after_s`: the log shows a `HOLD -> NORMAL:
released after ...` line followed by `NORMAL -> STOP`/`-> HOLD` lines each
time, and `/diagnostics` alternates between WARN (`HOLD (zero twist until
released by hold_release_after_s or reset)`) and the state of the brief NORMAL
interval.

Values other than `-1` or the ranges above fail configure. `/diagnostics`
reports both parameters.

## Other zero-twist cases

Besides STOP/HOLD, the plugin returns a zero twist (without failing the
controller) when: the pose or plan frame differs from the costmap frame; the
committed ray is blocked, unknown or off the costmap (legacy path); measured
progress is enabled and the pose is stale or the sample is invalid; the speed
limit is invalid; the core rejects its input; a candidate rollout is unsafe at
the commanded speed; or, with `people_stale_action: hold`, the people input is
stale. `last_command` in `/diagnostics` names the case.

## Other runtime details

- The decision interval is the measured time between control calls, with the
  published 0.1 s fallback on the first call (and after a clock regression or
  a stale-input hold). If the accumulator cannot reach `E0` at the nominal
  control period `1/controller_frequency` (`compass::is_non_vacuous` with
  `D_max = w_g + w_s + w_e + w_r`), configure logs a warning with the numbers.
- Costmap queries run under the costmap's mutex.

## Rebuild and recheck

From the repository root in a sourced ROS 2 Humble or Jazzy environment with
Nav2 (`nav2_controller`, `nav2_costmap_2d`) installed:

```bash
colcon build --base-paths src --packages-up-to compass_nav2 --cmake-args -DBUILD_TESTING=ON
source install/setup.bash
colcon test --packages-select compass_nav2 && colcon test-result --verbose
# Every controller_server key in both parameter files must be declared and applied
# (starts a headless controller_server per file; needs python3-yaml and rclpy):
bash scripts/check_param_binding.sh src/compass_nav2/config/compass_params.yaml sim/config/nav2_compass.yaml
```

CI runs the same check on Humble and Jazzy, plus two negative fixtures
(`scripts/param_binding_fixtures/`) that must fail. ROS 2 otherwise ignores
misspelled or retired keys silently.
