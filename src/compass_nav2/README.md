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
commanded motion is the published behaviour, with two exceptions that are on by
default. On Humble only, a new plan arriving after the control loop has been
idle longer than `task_gap_reset_s` resets the decision state (STOP/HOLD,
commitment, evidence, ρ and decision clock), as Jazzy's `reset()` does (see
[STOP and HOLD](#stop-and-hold-are-latched)). On both distributions, the local
goal is taken ahead of the robot along the plan instead of from the plan start
(see [Local goal](#local-goal); no change when the plan is re-issued at 1 Hz
from a pose up to 0.3 m behind the robot). Everything else added here only reports, or is an option
that is off by default.

| Parameter | Default | Meaning |
|---|---|---|
| `people_topic` | `"/people"` | `compass_msgs/People` input. A relative name resolves in the controller_server namespace (one tracker per robot). |
| `people_timeout_s` | `0.5` | People input is *stale* when no message arrived since configure or the latest is older than this. Age = max(steady-clock time since receipt, ROS-clock age of a non-zero header stamp). |
| `people_stale_action` | `"warn"` | `warn`: log and report only; the decision still uses the last message regardless of age, as the paper (§4.5) states. `hold`: zero twist while stale; `hold` is an adapter option beyond the published method. |
| `diagnostics_period_s` | `1.0` | Period of the `/diagnostics` status (wall timer, while active); `0` disables it. |
| `task_gap_reset_s` | `0.5` | Humble only, see [STOP and HOLD](#stop-and-hold-are-latched); `0` disables. Ignored on Iron and later. |

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
  `people_tf_last_failure_age_s`, `seconds_since_compute` and `last_command`
  (the last command and why it is what it is). Level WARN while STOP or HOLD is
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
been idle longer than `task_gap_reset_s` resets the decision state (STOP/HOLD,
commitment, evidence, ρ and decision clock), as Jazzy's `reset()` does at
action end. Idle time is measured from the moment the last control call
returned, so a call blocked inside the plugin (for example on the costmap
mutex) does not count. A mid-task stall without a new plan does not reset.
Humble's controller_server calls `setPlan()` when a FollowPath action starts
and, for goal preemption, inside the running loop within one control
iteration of the previous call; only the first can follow an idle loop. The
default 0.5 s is ten periods at the default 20 Hz, so in-loop updates never
qualify. Keep `task_gap_reset_s` well above the control period; the adapter
never uses less than three control periods (configure warns and says which
threshold it uses). In the default Humble behaviour tree, a retry that
typically restarts faster than that (the FollowPath `ClearLocalCostmap`
context recovery) keeps the latched state until a later recovery (Spin, Wait,
BackUp) opens a longer gap (reasoned from the Humble `controller_server` and
default BT source; not measured).
This differs from the paper's "HOLD retained until explicit external release"
only at task boundaries, as on Jazzy.

## Local goal

The decision core's local goal (`g` in the paper, `lookahead_dist` in the
parameter table) is the first plan pose that comes **after the robot's place on
the plan** and is at least `lookahead_dist` (straight-line distance) from the
robot; the last plan pose when there is none, and the point `lookahead_dist` in
front of the robot heading when the plan is empty. On the default (legacy) path
the direction from the robot to the local goal is the direction of the
clearance ray (the `d_safe` test) and of the corridor probe 1 m ahead, and the
local goal enters the goal cost. With `use_candidate_trajectories` it enters
the goal cost only.

The robot's place on the plan is the plan segment closest to it, found in two
steps. The plugin remembers the segment of the last cycle.

1. Forward search from the remembered segment: the segment, the next one, and
   later segments that start within 1.0 m of path length, repeated until
   nothing in range is closer. This is the only step while the robot is within
   1.0 m of the remembered segment, that is, while it follows the plan.
2. Re-acquisition: when the segment found by step 1 is more than 1.0 m from the
   robot, every segment of the plan is compared, and the nearest one (the
   earliest on a tie, the same segment the steering term uses) is taken if it
   is nearer by more than 1.0 m.

The poses before the robot's projection on that segment are never the local
goal.

| Event | Remembered segment |
|---|---|
| Control cycle that gets past the frame and people-freshness checks | Result of steps 1 and 2: the segment the robot is on. It stays where it was while the robot is up to 1.0 m behind it, and moves back only through step 2 |
| `setPlan()` with the same frame and the same x, y of every pose | Kept |
| `setPlan()` with any other geometry | Plan start |
| `reset()` (Iron and later), new task on Humble (`task_gap_reset_s`), configure | Plan start |
| Cycle stopped by those checks (pose or plan frame mismatch; `people_stale_action: hold` while stale) | Kept |

**This changes behaviour; it corrects a defect.** Until this change the search
for the first pose at least `lookahead_dist` away started at the first pose of
the plan. Once the robot was `lookahead_dist` (1.0 m) from that pose on a plan
that had not been replaced, the local goal was the plan start, behind the
robot: the ray and the probe looked backwards. An obstacle ahead on the path
then caused no braking until the robot's own cell was occupied, and an obstacle
behind the robot could cause braking.

- **Plan re-issued from near the robot pose at 1 Hz** (the default Nav2
  behaviour tree): the local goal is the same pose as before, so nothing
  changes. The old and the new rule select the same pose whenever every plan
  pose behind the robot is nearer than `lookahead_dist`. The robot may
  therefore travel `lookahead_dist` minus the distance by which the plan starts
  behind it before the rules differ: with the plan starting at the robot that
  is 1.0 m, 2.2 s at the default cruise speed of 0.45 m/s; with the plan
  starting 0.3 m behind the robot it is 0.7 m, about 1.5 s; with the plan
  starting 0.6 m behind it is 0.4 m, about 0.9 s, less than one 1 Hz period. Tested equal at
  1 Hz with the start up to 0.3 m behind the robot and poses snapped to 5 cm
  cells. Where the rules differ, the old one pointed backwards.
- **Plan set once** (a FollowPath client without periodic replanning), or
  replaced less often than that: while the robot follows the plan (stays
  within 1.0 m of it), the local goal is a plan pose after the robot's place on
  the plan in every cycle, from the first pose to the last, and the ray and the
  probe point at it. Near the end it is the last pose.

There is no parameter that restores the old rule.

Limits, and what happens in each case.

- **Reversing, or a pose that moves back along the plan** (relocalization):
  up to 1.0 m back, the remembered segment is kept and the local goal is the
  first pose from there that is `lookahead_dist` from the robot, so it is up to
  1.0 m further ahead than usual. More than 1.0 m back, step 2 finds the robot
  and the local goal is again `lookahead_dist` ahead of it.
- **Task restarted away from the plan start, or a pose that jumps forwards:**
  step 1 follows the plan to the robot when the plan keeps getting closer to it
  within each 1.0 m of path; otherwise step 2 finds it, provided the nearest
  segment is more than 1.0 m nearer than where step 1 stopped.
- **Plans that cross themselves or run back beside themselves:** the pass the
  robot has been following is kept while the other pass is not more than 1.0 m
  nearer. At a crossing the earlier pass is therefore kept. A robot that
  follows the later pass instead is moved to it once the earlier pass is more
  than 1.0 m further away; until then the local goal is on the earlier pass.
  Two passes less than 1.0 m apart are told apart by step 1 only: the forward
  search takes the later one when it starts within 1.0 m of path length and is
  closer.
- **A robot more than 1.0 m from every segment:** step 2 runs every cycle (one
  pass over the plan) and changes the segment only for one that is more than
  1.0 m nearer.
- **Bends tighter than `lookahead_dist`:** the first pose `lookahead_dist` from
  the robot in a straight line can be beyond the bend, to the side of or behind
  the robot heading. It is still a later pose of the plan.
- **Non-finite poses** (NaN or infinite x or y) are treated as absent: the
  segments join the finite poses on either side, and such a pose is never the
  local goal. A plan with no finite pose is treated as empty. Steering and the
  candidate rollout are unchanged: steering ignores segments that touch a
  non-finite pose, the rollout rejects the plan.
- The steering term and the goal-speed taper do not use the local goal:
  steering uses the segment closest to the robot over the whole plan in every
  cycle (no memory), and the taper uses the straight-line distance to the last
  pose.
- Pose and plan must be in the costmap frame, as before; nothing is
  transformed.

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
