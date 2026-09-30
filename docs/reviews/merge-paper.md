# Merge review: paper, implementation and evidence

Investigator: independent AI paper/code reviewer. This is not a human approval,
external peer review, deployment certificate, or main-merge authorization.

Initial read-only scope: local `9bd25aa` tree versus original main `9fe495a`.
After reporting findings, the integration coordinator assigned this investigator
the bounded remediation of `paper/arxiv/main.tex` and `paper/paper_draft.md`.
The coordinator owns PDF generation and the English Markdown entry point;
other investigators own results/provenance documentation, code and checkers.
A fresh combined-tree review is required after those changes are assembled.

## Initial findings and bounded remediation

1. **High: competing active theorem/evidence versions.**
   `paper/arxiv/paper_en.md` still claimed variance-only P1, deadlock escape P5,
   physical freezing and 50/50 intermittent observer censoring. Korean draft
   section 4.6 retained false reflected-state drift, correlated-input exponential
   claims, P4 boundary/forward-invariance omissions and a non-ceiling cycle bound.
   Canonical English LaTeX already contained substantially stronger conditions.
   Remediation: canonical LaTeX entry point assigned to coordinator; Korean
   critical theory replaced by a faithful conditional summary, its literature
   duplication narrowed, and obsolete revision notes explicitly labelled historical.

2. **High: pseudocode and safety claims contradicted the current contract.**
   Original `main.tex` algorithm counted an unbounded integer, omitted absorbing
   HOLD/release and empty-challenger handling, and updated progress after the
   safety branch. Safety prose conflated intervention and actual class switches,
   asserted global monotone descent, and called branch priority a safety guarantee.
   Remediation: rolling open-left event deque, once-per-time recording, HOLD
   output, no-challenger branch, safety early return and actual progress resets
   are explicit. The final code repair uses interval-scaled acceleration and
   resets `L_real` on safety/HOLD commitment changes, reflected in both papers.
   P3/P5 remain software priority/escalation contracts, not collision avoidance.

3. **High: synthetic comparator was presented as a causal ablation.**
   `harness.hpp::run_noclass` is coin-flip label flicker within a cost band and
   otherwise argmin; it also removes the accumulator. The former abstract,
   results interpretation and figure caption treated it as isolated correspondence
   removal. Remediation: identify the historical CSV key as a synthetic stress
   comparator, distinguish knob-only ablations from separate policies, and remove
   causal correspondence claims. Existing raw decision counts are not changed.

4. **Medium: corrected observer tables coexisted with stale prose and an
   incomplete metric definition.** Original `main.tex` results interpretation and
   Korean prose said 48/250 for both oscillatory comparators; current CSV gives
   72/250 and 71/250. Setup omitted the new 0.30 s observed-follow-up requirement.
   Remediation: corrected prose; explicitly distinguish assumed likelihood error
   from sampled corruptions, decision-stream scoring from motion/human validity,
   and probability-space, log-odds-only, and log-odds-plus-follow-up versions.
   Follow-up is a metric-definition change, not merely a floating-point repair.

5. **Medium: theorem/evidence scope and provenance were easy to overread.**
   The first `n`-cycle stationary P1 counterexample is valid, but the old claim
   that reset-on-switch marginals converge to 1/2 is not. The Gaussian scripted
   input is not almost-surely bounded, and switch counts exclude initialization
   to first output. R1/R5 scripted decisions, historical R3 latency, the 3,750-run
   sweep, the deterministic unicycle diagnostic and new candidate integration
   are different evidence sets. Remediation: first-crossing counterexample,
   explicit update-clock/initial-state qualification, shared output-transition
   convention, unbounded-harness caveat, and separate opt-in/evidence scope.
   Default input-order top-K labels do not prove an integrated TTL/group lifecycle;
   the one-second rollout does not prove label homotopy or the ideal G2 interface.

6. **Medium: normalization prose had concrete algebra/unit errors.**
   Narrower min-max range requires a smaller raw difference, fixed constants can
   co-transform affine units, and multiplying by seconds gives evidence and its
   threshold normalized-cost-second units. Normalized range alone supports P2;
   time-invariant semantic calibration is distinct. Both papers now state the
   clipped formula and actual conditions without changing knobs or data.

## Issue mapping

- **#3:** observer saturation and positive-follow-up definitions are separated;
  historical values and hashes are owned by the provenance investigator.
- **#9:** all six streams share the adjacent-output transition convention;
  first output is excluded uniformly, and unclipped Gaussian inputs are not
  presented as bounded-theorem validation. Synthetic comparator is not causal.
- **#11:** P4 uses a ceiling and explicit forward-invariance scope; P1 separates
  pre-test/stored evidence and uses a first-crossing stationary counterexample.
  Leaky positive-input hitting uses the geometric response expression, not the
  pure-integrator linear ceiling.

## Checks and disposition

Read-only initial checks all passed despite stale prose, showing why table-only
or numeric-substring gates did not establish whole-document consistency. After
remediation, `check_paper_numbers.py`, the strengthened labelled-cell
`check_observer_results.py`, `check_switch_bounds.py` (2,000 response cases and
2,000 variable-period traces plus boundary cases), and `git diff --check` pass.
PDF build/render QA and final independent combined-source review are coordinator
gates, not claimed complete by this record.

These corrections support a bounded **research/software merge** after final
software and artifact checks; they do not support a production or physical-safety
claim. Unexecuted Gazebo/robot trials, independently justified opportunity/free-
motion oracles, physical stopping and collision behavior, external baselines,
human/motion legibility, and the 480 planned physical cases remain separate
validation gates. Merely reducing output-label changes does not close them.
