# Pre-merge core/Nav2 code investigation and remediation

This is an AI investigator's review and implementation record, not independent
human approval, robot validation, or authorization to merge/deploy. Initial
read-only review used integrated tree `9bd25aa` (reported tree-equivalent to PR
#15 source `1f17ba42`) against main `9fe495a`. The coordinator subsequently
authorized narrowly scoped fixes and supplied open issues #4, #6 and #8.

## Findings fixed in this pass

1. **P1 — old maneuver progress survives safety commitment (issue #8).**
   `src/compass_core/include/compass_core/decision_state.hpp`,
   `src/compass_core/src/decision_core.cpp` safety/HOLD branches.
   Initial executable reproduction: seed `L_real=rho=.7`, safety-switch R to L,
   then submit a stationary measured sample. The switching tick had `rho=0`,
   but the stationary tick restored `rho=.7` from the previous maneuver.
   Shared safety reset and HOLD re-commit now clear `L_real` in both measured
   and historical proxy modes. Braking without a commitment change retains
   current progress. Preserving this erroneous runtime state was explicitly
   rejected by the coordinator; unchanged no-safety archived metrics are the
   compatibility target.

2. **P1 — invalid coordinate-frame comparisons could emit commands.**
   `src/compass_nav2/src/compass_controller.cpp`, command entry frame guard.
   Initially, only measured-progress mode checked pose/plan frames. Candidate
   mode alone, and legacy mode, could interpret a map-frame plan as odom-frame
   coordinates. All command paths now require pose and nonempty-plan headers
   to match the configured costmap frame and return a complete zero twist before
   modifying decision state on mismatch. This is rejection, not a new TF/plan
   transformation implementation.

3. **P1 — sampled static clearance was not a swept lower bound.**
   `src/compass_nav2/src/costmap_env_query.cpp`, `trajectoryClearance`.
   Concrete reproduction: a 1 m/s straight rollout samples x=0 and x=.05; a
   lethal 1 mm cell at x=[.025,.026], y=[.2499,.2509] overlaps the declared
   radius-.25 robot between samples. The original code returned clearance
   `0.000888621` and accepted `executionSafe(..., d_safe=0, ttc_min=0)`.
   Subtracting half the largest interval arc length from the sampled minimum
   provides a conservative static-distance bound for the exact unicycle model.
   The reproduction now returns zero/unavailable. The subtraction also covers
   map edges; intervals too large for the bounded scan fail closed.

4. **P1 — braking acceleration was applied as a per-call decrement (issue #4).**
   `src/compass_core/src/safety.cpp`, `safety.hpp`, core safety call site.
   The previous `.45 - .5` calculation stopped a .45 m/s command in one call,
   despite the documented acceleration contract. The safety API now requires
   explicit positive `dt`; braking is `max(0, v-a_brake*dt)` with finite,
   nonnegative acceleration. `.45 m/s`, `.05 s`, `.5 m/s^2` produces `.425 m/s`;
   `.10 s` produces `.4 m/s`. Zero/small bounds, STOP/HOLD, ordinary startup and
   candidate changed-speed revalidation remain tested. This changes runtime
   safety behavior intentionally and requires rebuilding API consumers.

5. **P1 — legacy environment used reversed TTC and permissive map fallbacks.**
   `src/compass_nav2/src/costmap_env_query.cpp`, `occupied`, `clearance`,
   `feasible`, `ttc`; controller zero-clearance command guard.
   The documentation investigator independently reported these findings and the
   code investigator confirmed them. With a person 1 m ahead and a robot moving
   toward them at .5 m/s, the old relative-velocity sign returned the default
   TTC instead of 2 s; the reverse case could look approaching. The sign is
   corrected. Missing maps, unknown cells and out-of-map points are unavailable;
   the current robot center is checked before testing offset rays. An immediately
   unavailable/blocked legacy ray now produces a full zero twist, not a positive
   braking command or an angular-only command.

6. **P1 — legacy side checking was mirrored relative to commands.**
   `src/compass_nav2/src/costmap_env_query.cpp`, `lateral_bias`.
   R previously selected a positive-normal ray while the emitted tracker used
   negative-normal bias. The ray convention is now L-positive/R-negative,
   matching candidate rollout and emitted path-tracking commands. A one-sided
   obstacle regression detects the previous mirrored result. This is not a
   claim that the legacy ray predicts the entire executed trajectory.

7. **P2 — mathematical-domain violations could return plausible bounds (issue #6).**
   `src/compass_core/src/accumulator.cpp`, `accumulator.hpp`, core constructor
   and decision entry; controller exception-to-zero guard.
   Examples included lambda>1 silently taking the lambda=1 reachability branch,
   nonpositive dt, and invalid threshold/cap parameters. The core/public helpers
   now reject nonfinite/out-of-domain parameters with `invalid_argument`:
   lambda in (0,1], E0>0, nonnegative finite caps/margin/hardening, p>=1, positive
   finite dt, rho in [0,1], and finite D_max>delta_floor for the positive-advantage
   reachability helpers. Valid thresholds are >=E0. The k_rho=0 ablation remains
   supported; `rho_bar=1` is documented as a neutral sentinel, not proof of
   progress-dependent blocking. The command adapter catches these contract
   errors and emits zero. Lambda remains per-update: variable dt changes its
   physical leak time. Nine reset fixtures verify 3/6/9-cycle lower bounds after
   challenger, safety and discretionary resets; these are not legibility tests.

## Verification and handoff

- `scripts/test_completion_gates.sh` passes: four standalone C++ contract
  executables, three scripted-cost diagnostic profiles, eight physical-metric
  scorer tests and sixteen opportunity tests. Diagnostic outcomes remain the
  proxy timeout and identical measured/profile goal times of 17.45 s; this is
  not Nav2/Gazebo or physical evidence.
- The safety/domain executable includes five progress-epoch fixtures, nine reset
  timing fixtures and invalid-domain rejection checks. It and the velocity
  contract were also compiled with `-DNDEBUG`; assertions remain active.
- Eight environment test fixtures pass a bounded local build of the actual
  environment source using minimal Costmap2D/assertion stand-ins. This confirms
  the concrete geometry/arithmetic reproductions but is **not ROS or gtest
  runtime evidence**. No stand-in files are added to the repository.
- Registered ROS tests now include eight environment fixtures and four
  plan/controller fixtures (republish, four frame-mismatch variants, core-error
  containment, legacy missing-map command). A shared state-reset gtest was added.
  Their real ROS Jazzy run remains required at the final integrated commit.
- Both opt-in defaults and default knob values remain unchanged. Corrections to
  known unsafe legacy runtime behavior are intentional; the coordinator owns
  the full archived-data regression, version bump, final CI and fresh review.
- `git diff --check` passes. This investigator made no commits or remote writes.

No remaining defect from the bounded findings above is knowingly left unfixed.
Fresh review and final-source CI are still merge gates. Legacy ray geometry,
constant-velocity people prediction, fixed circular proxies, sensing freshness,
braking/actuator realization, general multi-person homotopy and physical
performance remain explicitly unvalidated research/deployment limitations.
HOLD remains absorbing until explicit release/reset; no automatic physical
recovery, robot stopping guarantee or human motion-legibility claim is made.

## Fresh-review addendum: covariance symmetry

A subsequent independent AI code reviewer found one additional opt-in contract
defect in `cost_evaluator.cpp`: covariance `{1,100,-100,1}` was averaged into an
apparently valid diagonal matrix and received finite cost. The candidate path now
rejects asymmetry before averaging, permitting only numerical roundoff satisfying
`|c01-c10| <= 1e-12 * max(1, |c01|, |c10|)`. The standalone candidate-cost regression
covers the reported cancellation, valid symmetric PSD input, inside/outside
tolerance, and unchanged legacy no-trajectory behavior. This fix is confined to
the optional candidate evaluator; the reviewer must recheck it and final-source
CI remains mandatory.
