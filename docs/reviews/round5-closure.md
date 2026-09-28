# Round-5 closure record (issues #3–#11)

Scope: gate-by-gate status of the round-5 issues after the follow-up fixes on
branch `fix/round5-open` (base `d543dc2`, PR #15 head). This is an author-side
record, not a review verdict. It does not close any issue; the maintainer decides
whether to close, keep open or split each issue.

Status values: **MET** (evidence in the tree), **MET (alternative)** (the
issue's stated alternative was taken), **PARTIAL**, **OPEN** (not met, reason
given), **EXTERNAL** (needs data, hardware or people not available to this
repository), **PENDING** (assigned to a separate reviewer).

Commits on `fix/round5-open`: `34f7075` (#3), `ebbebe5` (#6), `3da723e` (#7),
`e9c305e` (#8), `dbed2bd` (#10), and the commit adding this file (#11).
Earlier evidence is at `d543dc2` as audited in the round-5 issue audit.

## #3 R5-01 observer saturation and result versions

| Gate | Status | Evidence |
|---|---|---|
| R30→L170 and R60→L140 give 3.05 s / 6.05 s, uncensored | MET | `src/compass_eval/test/test_observer.cpp` |
| R100→L100 stays censored | MET | same |
| R/L mirror identical; no saturation after a long hold then reversal | MET | same (long-hold case added in `34f7075`) |
| Same 1,500 streams compared under old/new metric; raw results and commands recorded | MET | `34f7075`: `src/compass_eval/results/observer_versions/` (three raw CSVs, per-row `observer_versions.csv`, commands), `scripts/check_observer_versions.py` |
| Mean, SD, censoring, per-scenario tables and figures regenerated | MET | `R1_ablation.md`, `scripts/check_observer_results.py`; the oscillation figure uses switch counts only, which are unchanged |
| §5.3 intermittent cause corrected; physical-legibility limit kept | MET | `paper/arxiv/main.tex` §5.3 |
| Stale README lines replaced, not appended | MET | `34f7075`: `src/compass_eval/results/README.md`, `README.md`, `docs/reviews/round5-observer.md` |
| Preregister a positive hold, or rename the endpoint-suffix statistic and drop fixed-hold/conservative claims | MET (alternative) | `34f7075`: statistic named `t_sfx`; 0.30 s rule labelled post hoc (chosen after the PR #12 review); "strictest"/conservative/fixed-hold claims removed. A preregistered hold was not made and cannot be made retroactively. |
| Metric-definition change kept separate from the saturation-only fix; both versions retained | MET | `34f7075`: `observer_versions/` (9fe495a, 5343d45, current); 200 rows change with the numerical fix, 47 more with the follow-up rule |

## #4 R5-02 zero-valued safety speed limits

| Gate | Status | Evidence |
|---|---|---|
| Zero/small caps, 0.425 cap, STOP/HOLD zero, NaN handling | MET | `d543dc2`: `velocity_limits.hpp`, `compass_controller.cpp`, `src/compass_nav2/test/test_velocity_limits.cpp` |
| Residual: tests exercise the limit helpers, not a full `computeVelocityCommands` Twist | PARTIAL (minor) | recorded in the round-5 audit; ROS runtime not available here |

## #5 R5-03 P5 event counting and termination scope

| Gate | Status | Evidence |
|---|---|---|
| Open-left window, once per timestamp, absorbing HOLD, release; narrowed P5 and no-termination remark | MET | `d543dc2`: `safety.cpp`, `src/compass_eval/test_safety_window.cpp`, main.tex Prop. P5 |

## #6 R5-04 P2 for fixed and variable intervals

| Gate | Status | Evidence |
|---|---|---|
| Pre-test/post-reset notation and variable-interval P2 | MET | `d543dc2`: main.tex Prop. P2 |
| 3-cycle figures only for fixed dt = 0.05 s; variable dt via accumulated time or validated dt_max | MET | `ebbebe5`: Remark `rem:vardt` (no validated h_max exists for the adapter, so only the accumulated-time form is claimed for it) |
| Domain validation (λ, dt, E0, e_max, D_max, threshold) | MET | `d543dc2`: `test_safety_window.cpp` |
| N_min ≥ 2 not presented as legibility | MET | P2 statement; related-work "renders motion legible" removed in `ebbebe5` |
| 3, 6, 9-cycle reproduction; retarget/reset never advances the earliest switch | MET | `d543dc2`: `test_safety_window.cpp` |
| Leak time changes with dt; per-second λ_k = exp(−dt_k/τ) as a separate change | MET | `ebbebe5`: Remark `rem:vardt`, comment at the adapter dt computation. Not implemented; the adapter still enforces no h_max. |

## #7 R5-05 response-time reachability and clip lockout

| Gate | Status | Evidence |
|---|---|---|
| No-reset/no-HOLD/full-update hypotheses | MET | `d543dc2`: Prop. `p2live` |
| Clip lockout at ρ > √(2/3) | MET | Remark `rem:resptime` |
| 76/24/17/7 cycles | MET | Remark `rem:resptime`, `scripts/check_switch_bounds.py` |
| Clip equality vs equilibrium equality | MET | §4.9, `check_switch_bounds.py` exact-fraction case |
| "Lower λ improves responsiveness" removed | MET | `d543dc2` |
| Noiseless ramp separated from the Gaussian experiment; no no-switch proof from a window shorter than N_resp | MET | `3da723e`: §5.3 `mid_reversal` rewritten; clip lockout stated as a separate mechanism; "liveness condition" wording replaced by reachability headroom (P1, P4, §4.9, §5.5, conclusion) |

## #8 R5-06 realized cross-track progress

| Gate | Status | Evidence |
|---|---|---|
| Straight forward motion does not increase ρ | PARTIAL | MET in opt-in measured mode (`test_measured_progress.cpp`); the default forward-speed proxy still increases ρ with forward speed |
| ρ follows lateral progress and retreat; signed L/R consistency | MET (opt-in) | `test_measured_progress.cpp`, `src/compass_eval/test_response.cpp` |
| Anchor/L_real/L_plan consistent on ordinary/safety commit and episode reset | MET (helper level) | `d543dc2`: `test_safety_window.cpp` epochs; `e9c305e`: L_plan anchored per epoch. The controller wiring is not compiled or run here (ROS Jazzy CI job) |
| Zero planned offset, plan replacement, localization jump | MET (helper level) | `e9c305e`: `planned_offset` tests, core zero-offset test; jump invalidation already tested |
| L_plan is the planned lateral offset, not a constant | MET (opt-in) | `e9c305e`: tracker equilibrium `k_side/k_e` minus anchor offset; `progress_length` knob removed; model unvalidated |
| v_lat sweep kept as synthetic sensitivity, separate from corrected-estimator closed loop | MET (text) / EXTERNAL (evaluation) | `e9c305e`: paper §4.5, README roadmap; no closed-loop run of the corrected estimator exists |
| P4 only under realized progress; ρ estimate 1 is not maneuver success | MET | `e9c305e`: remark "Estimated versus realized progress" |
| Default ρ input switched to realized progress | OPEN | Not done. The offline harness has no pose or geometry, so a "measured" default there would be a scripted increment numerically identical to the proxy, and switching the adapter default cannot be built or validated without ROS/Gazebo. No results were regenerated; all archived results are labelled as proxy results. |

## #9 R5-07 ablation attribution

| Gate | Status | Evidence |
|---|---|---|
| Rename/scope instead of an identity-only ablation; uniform switch convention; bounded-input scope; common-prior claim removed | MET (alternative) | `d543dc2`: synthetic correspondence-loss comparator naming, candidate-path tests, observer paragraph |

## #10 R5-08 warranted-switch observability and closed-loop gates

| Gate | Status | Evidence |
|---|---|---|
| Oracle independent of the algorithm's evidence threshold | MET (definition) / EXTERNAL (oracle) | `dbed2bd`: §5.6 definition; `scripts/evaluate_opportunities.py` takes external labels; no independently annotated oracle exists |
| Onset/expiry, alternative, eligibility, deadline preregistered | PARTIAL | schema and protocol fields defined (`dbed2bd`, `RESPONSIVENESS.md` pending 480-case plan); concrete per-scenario values are not yet fixed |
| Theorem validation and application recall reported separately; exclusions disclosed | MET (definition) / EXTERNAL (measurement) | `dbed2bd` |
| No-response censored at the deadline; no success-only averaging | MET | `dbed2bd`; scorer contract in `d543dc2` |
| Lockup as deadline-bounded failure while target and safe alternative persist | MET | `dbed2bd` (replaces "never taken") |
| Physical freezing by speed, goal progress, duration and yield/HOLD separation; not equal to zero switches | MET (definition) / EXTERNAL (trials) | `dbed2bd`; `scripts/physical_metrics.py` provisional thresholds |
| No physical performance claim before success/collision/clearance/freezing data | MET | §5.6 |
| Integrated robot logging, closed-loop and physical trials | EXTERNAL | no robot, Gazebo run or human study available |

## #11 R5-09 regeneration and closure review

| Gate | Status | Evidence |
|---|---|---|
| Base SHA, compiler/flags, seeds, parameters, commands, raw data hashes | MET | this commit: `src/compass_eval/results/PROVENANCE.md`, checked by `check_repo_consistency.py` |
| Tables and figures from raw; mean/SD/censoring consistent | MET | `check_observer_results.py`, `check_repo_consistency.py`, `check_paper_numbers.py` |
| Observer repair, algorithm repair and metric change separated | MET | `observer_versions/` (streams identical across versions) |
| P4 ceil, P1 leaky hitting time, P1 pre-reset notation | MET | `d543dc2`: `docs/reviews/round5-theorems.md` |
| Number checker, PDF compile, direct figure/table/claim review | MET (author side) | canonical PDF rebuilt with `scripts/build_paper.sh --write` at every fix commit; author spot check only |
| Fresh adversarial review naming the modified SHA | **PENDING** | to be done by a separate fresh reviewer; this record is not that review and does not self-certify |
| PR/issue tracker state reconciled with the ledger (e.g. PR #1 body) | OPEN | requires maintainer edits on GitHub; not done from this branch |

## Summary of what remains open

- #3: no preregistered hold (alternative taken; cannot be retroactive).
- #6: no enforced or validated dt upper bound in the adapter; per-second leak not implemented.
- #8: default ρ input is still the forward-speed proxy; corrected estimator not evaluated in closed loop; planned-offset model and adapter wiring not validated on ROS.
- #10: independent oracle, robot logging, closed-loop and physical freezing trials (external).
- #11: fresh independent closure review (PENDING); GitHub state reconciliation (maintainer).
