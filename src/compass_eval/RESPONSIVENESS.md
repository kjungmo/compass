# Opt-in responsiveness experiment

This is a research configuration, not the default parameter profile.
`responsive_profile()` sets only `k_rho=0.5`. The existing accumulator, challenger
reset, tie breaking and safety branches remain in use. Default `Knobs{}` values
and the legacy progress fallback are retained. The current 1,500-row canonical
CSV uses the corrected log-odds observer and is not byte-identical to the
original `9fe495a` archive; label switch counts, sign-change rates and entropy
are unchanged in that battery. Integrated safety repairs are separate behavioral
changes, so this is not a claim of global old/new state equivalence.

## Why this candidate

| Candidate | Effect | Limitation |
|---|---|---|
| Correct progress input only | Stops forward travel being counted as lateral progress | Real high progress still makes threshold exceed the cap |
| Raise cap only | Removes the cap barrier | At D=.40, dt=.05, leak equilibrium .5833 remains below threshold .60 |
| Remove progress hardening | Maximum threshold .30 is reachable | Removes the progress-dependent preference |
| Chosen: k_rho=.5 plus measured input | Maximum threshold .45 < cap .50; retains hardening | Changes permanent-blocking properties; weaker/noisy advantages need separate evaluation |

For a fixed safe challenger with advantage D >= .40 each cycle, dt=.05,
lambda=.97, delta=.05, zero initial evidence and no challenger/safety resets,
the uncapped trajectory is e_n=a(1-lambda^n)/(1-lambda), a=.0175.
The worst threshold over rho in [0,1] is .45, below both the .50 cap and .5833
leak equilibrium. The crossing formula gives 49 cycles; the helper returns a
conservative 50-cycle bound to cover integer rounding. This is a conditional
bound, not a guarantee whenever an external oracle wants a switch. Noise that
violates the sustained lower bound does not satisfy the theorem premise.

The old P4 interior permanent-blocking conclusion does not apply to this profile
for such advantages. The deterministic minimum-crossing result (P2) uses the new
knobs with its range-bound and reset hypotheses; its companion response-time
result additionally requires persistent challenger selection/eligibility and
clip/leak headroom. The probabilistic refinement (P1) also requires the declared
i.i.d. increment assumptions. The integrated manuscript has separately received
theorem corrections; this profile's bound must not be applied without its own
hypotheses. Keep the original parameter configuration when reproducing the
paper's default-policy experiments.

## Input and integration contract

`DecisionInput::lateral_progress_delta_m` is an optional signed displacement in
metres along the CURRENT commitment, measured over the preceding interval.
The adapter must project realized displacement onto the committed lateral
maneuver direction in a consistent frame. Reverse motion may reduce progress.
Do not pass command speed, total traveled distance or a tracker ID as progress.
The denominator `DecisionState::L_plan` must describe the same maneuver.

`nullopt` keeps the legacy abs(forward speed)*dt approximation. The explicit
`step_evals(..., trace, progress_delta_m)` overload is available to adapters and
tests. Zero is valid measured progress; it never falls back to command speed.
Nonfinite deltas and nonpositive/nonfinite dt are rejected before state mutation.
A discretionary commit discards the previous commitment's interval increment;
safety returns skip that interval's progress increment. A safety switch or
HOLD-entry recommit to another class clears cumulative distance and rho in both
proxy and measured modes;
the next stationary sample must not import the previous maneuver's progress.
Negative progress is clamped at cumulative zero. A stale/missing measurement
must be handled explicitly by the caller rather than quietly replaced by speed.

The source version is 0.2.0: public layouts and the environment virtual interface
have changed, so rebuild all dependent binaries. Existing core entrypoint symbols
are not an ABI guarantee. The integrated ROS wrapper now has an estimator behind
`FollowPath.use_measured_progress=false`; it projects stamped realized pose
displacement onto a frozen path normal. Set `FollowPath.k_rho=0.5` separately in
the configuration to select the research responsiveness knobs. Neither option
enables the other, and parameters are read at controller configuration time.

The estimator's declared defaults are `progress_max_gap=0.25 s`,
`progress_max_speed=2.0 m/s`, and `progress_length=1.0 m`; these are proposed
limits, not calibrated measurements. Invalid intervals produce a zero command.
Identical XY geometry/frame republishes preserve the interval, while changed
geometry/frame starts a new one. Mixed equally split side classes are unsupported.
The frozen normal is a coherent-maneuver proxy, not a general multi-person
progress estimator. No Gazebo validation or robot deployment is claimed.

## Reproduce and inspect

```sh
bash scripts/test_compass_observability.sh
bash scripts/test_compass_response.sh /tmp/compass_response_results
# Explicit measured zero progress, despite forward input .5:
compass_trace clean_commit 0 .5 trace.jsonl responsive 0
# 480 pending physical cases, not results:
python3 scripts/generate_physical_pilot.py /tmp/physical_pilot.json
```

Standalone C++17 -O2 validation uses g++/libstdc++; see results/responsive_profile.
The paired sweep covers 5 scenarios x 5 inputs x 50 seeds x 3 configurations =
3,750 runs, 200 cycles each. It compares legacy/proxy, responsive/proxy, and
responsive/measured with a SCRIPTED constant measured rate equal to input.
It does not validate a real progress estimator. The same generated per-seed
scenario inputs are used across profiles; the sampler is unchanged.
`paired_results.json` retains each seed's switches and clean-case recall; the
runner emits the full CSV including trace hashes. This sweep does not overwrite
the default-policy CSV. The checked-in validation transcript describes its earlier
run, including the then-current observer baseline and test counts; it is not the
final integrated CI record. Reruns need fresh source/config/toolchain provenance.

Clean-case oracle: target L, onset 0, confirmation .5 s, deadline 3.5 s, hold .3 s,
expiry 10 s. Both responsive configurations achieve 50/50 timely responses at
all five inputs; legacy achieves 50/50 at .05/.10/.20 and 0/50 at .35/.50.
Near-tie and transient-spike switch counts stay zero in all tested configurations.
Mid-reversal also stays zero: no oracle is provided for this scenario, and this
must NOT be treated as proven warranted responsiveness. Intermittent mean
switch count rises from .4 to .6 across the input sweep; an independent oracle
is needed to decide whether these extra changes are useful. No global
oscillation non-regression or physical safety claim follows from this suite.

## Physical pilot: 480 planned cases

8 encounters x 20 paired seeds x 3 configurations (legacy proxy, legacy measured,
responsive measured). Encounters: narrow/wide head-on, narrow-to-wide,
wide-to-narrow, human/robot overtaking, late side reversal, safe wait then clear.
Each trial has a 60 s horizon. The manifest deliberately marks every case pending
and leaves geometry/actor/oracle/source/config hashes empty until real fixtures
are selected. A simulator adapter must refuse to run unbound fixtures. Keep the
same geometry and recorded actor inputs for matched seeds; a seed alone is not
sufficient to establish identical input data.

Collect BOTH decision JSONL and independent motion JSONL with a shared clock.
Motion rows summarize nonoverlapping intervals [t,t+dt), beginning at cycle 0:
`t,dt,cycle,speed_mps,goal_distance_m,min_clearance_m,free_motion_warranted,
planned_stop,collision,goal_reached`. Speed is a conservative upper bound on
realized speed during the interval; clearance is the interval minimum; distance
must stay outside goal tolerance for a freezing interval. Event flags capture
any interval collision and validated terminal goal. The free-motion oracle must
be independent of COMPASS's cost and selected label; log its evidence/version.

`physical_metrics.py` identifies continuous >=2 s intervals with speed <=.02 m/s,
goal distance >.3 m, clearance >=.5 m, independently warranted motion and no
planned stop/collision/goal. It reports the whole qualifying episode duration.
These thresholds are proposed evaluation definitions, not validated universal
robot constants. Gap/truncated timeout logs reject rather than count as success.
A success requires goal termination, terminal goal flag, distance <=.3 m and no
logged collision. Missing/pending trials are not failures and are not in a rate
denominator. Aborts must be reported separately by cause before aggregate rates.

Report per-run freeze duration, success/collision, goal time and opportunity
recall alongside output-label switches. For matched comparisons use scenario-
stratified paired intervals and show denominators, invalid logs and aborts.
No physical trials, geometry oracle, adapter, confidence-interval aggregation or
human/motion legibility study was executed here. These are the remaining gates
before claiming that the physical freezing/success gap is closed.
