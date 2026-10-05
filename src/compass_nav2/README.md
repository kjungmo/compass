# compass_nav2 — deployment notes

`compass_nav2::CompassController` is the Nav2 controller plugin around the
`compass_core` decision layer. This page covers what an operator needs when
running it under `controller_server`. It does not change the research scope
stated in the [root README](../../README.md): the plugin is a research
implementation, not a qualified product.

Example configuration: [`config/compass_params.yaml`](config/compass_params.yaml).
Knob semantics are in the paper and in `compass_core/knobs.hpp`.

## Parameters added for deployment

All are under the plugin name (`FollowPath.` below). Defaults reproduce the
earlier behaviour except where noted.

| Parameter | Default | Meaning |
|---|---|---|
| `people_topic` | `"/people"` | `compass_msgs/People` input. A relative name resolves in the controller_server namespace (one tracker per robot). |
| `people_timeout_s` | `0.5` | People input is *stale* when no message arrived since configure or the latest is older than this. Age = max(steady-clock time since receipt, ROS-clock age of a non-zero header stamp). |
| `people_stale_action` | `"warn"` | `warn`: log and keep deciding with the last message (the earlier behaviour). `hold`: zero twist while stale. |
| `diagnostics_period_s` | `1.0` | Period of the `/diagnostics` status (wall timer, while active); `0` disables it. |
| `task_gap_reset_s` | `1.0` | Humble only, see below; `0` disables. Ignored on Iron and later. |

Why 0.5 s: it is a quarter of the default `ttc_min` (2.0 s), and a person at
1.4 m/s closing head-on with the robot at 0.5 m/s moves about 0.95 m in that
time, more than the default `d_safe` (0.5 m).

Every knob is range-checked at configure; an out-of-range value fails the
configure transition with a message such as
`FollowPath.d_safe = -0.5 is outside the allowed range [0, inf)`.

## What the operator sees

- **Mode changes** are logged: `NORMAL -> STOP` (warning, with the committed
  class's TTC against `ttc_stop`), `NORMAL -> HOLD` (warning, with the
  intervention count in `W` against `n_thrash`), and the return to `NORMAL` on
  reset.
- **`/diagnostics`** carries one `DiagnosticStatus` named
  `"<node name>: compass"` (`hardware_id` = node namespace) with `mode`,
  `committed_class`, `e_rev`, `rho`, `people_topic`, `people_count`,
  `people_age_s`, `people_stale`, `people_applied_last_cycle`,
  `people_tf_failures`, `seconds_since_compute` and `last_command` (the last
  command and why it is what it is). Level WARN while STOP or HOLD is active or
  the people input is stale or missing; OK otherwise. It is published from a
  wall timer, so it keeps arriving when the control loop or `/clock` stops.
- **People TF failures** (people frame cannot be transformed into the costmap
  frame) are logged with the reason and counted; that cycle decides with no
  people, as before.
- All throttled warnings use a steady clock.

## STOP and HOLD are latched

As published, STOP and HOLD are absorbing: once entered, the plugin returns a
zero twist until the decision state is reset, even after the person has left.
Nav2's progress checker then aborts the goal. On Iron/Jazzy and later,
controller_server calls `reset()` when a task ends, so the next goal starts in
NORMAL. Humble has no such hook; there the plugin treats a pause between control
calls longer than `task_gap_reset_s` as a new task and resets the same state.
Retries sent sooner than that keep the old state, and a control loop stalled
for longer resets mid-task.

## Other zero-twist cases

Besides STOP/HOLD, the plugin returns a zero twist (without failing the
controller) when: the pose or plan frame differs from the costmap frame; the
committed ray is blocked, unknown or off the costmap (legacy path); measured
progress is enabled and the pose is stale or the sample is invalid; the speed
limit is invalid; the core rejects its input; a candidate rollout is unsafe at
the commanded speed; or, with `people_stale_action: hold`, the people input is
stale. `last_command` in `/diagnostics` names the case.

## Other runtime details

- The first decision after start or reset uses one controller period
  (`1/controller_frequency`) as its interval. If the accumulator cannot reach
  `E0` at that period (`compass::is_non_vacuous` with
  `D_max = w_g + w_s + w_e + w_r`), configure logs a warning with the numbers.
- Costmap queries run under the costmap's mutex.
- CI checks on Humble and Jazzy that every `controller_server` key in
  `config/compass_params.yaml` and `sim/config/nav2_compass.yaml` is declared
  and applied ([`scripts/check_param_binding.sh`](../../scripts/check_param_binding.sh));
  ROS 2 otherwise ignores misspelled or retired keys silently.
