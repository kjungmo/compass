# COMPASS — current English manuscript

The canonical English manuscript is [main.tex](main.tex); the corresponding
typeset reading copy is [main.pdf](main.pdf). This entry replaces the independently
edited English Markdown duplicate so that retired claims cannot silently diverge
from the reviewed manuscript. The old Markdown remains available in Git history
at the archived baseline `9fe495a7f12e06cd2a3da46d0a3aa7f1db92859f`.

## Current scope

COMPASS is a research decision layer for temporally consistent passing labels.
The headline P2 bound limits discretionary switching under its stated assumptions;
the response deadline is conditional on persistent challenger selection and
eligibility, uninterrupted updates, sustained advantage, and clip/leak headroom.
P1 is an independence-conditioned probabilistic refinement. P4 concerns a
conditional evidence-blocking region, and P5 is a finite-window escalation to
absorbing HOLD, not a physical deadlock-escape guarantee.

The commitment rule is a rectified, leaky variant of Page's one-sided CUSUM
statistic with a progress-dependent threshold, under a lexicographic safety
override of the standard priority-arbitration kind. P1 and P2 apply standard
CUSUM/queueing and dwell-time arguments, P3 and P5 are branch-priority and guard
contracts, and P4 is a progress-rate and accumulator-containment argument; none is
claimed as new mathematics. Track-ID-indexed classes are a design contract that
the current implementation only partly realizes. Previous-class consistency
weights (de Groot et al.; SHINE), opinion-dynamics passing (Cathcart et al.) and
dwell-plus-relative-improvement selection in concurrent work (BIG-CBF) are the
closest prior mechanisms; none of them is compared here, and all comparators are
internal. P2's worst-case separation at the default knobs is 3 cycles (0.15 s),
shorter than the 12 cycles (0.6 s) enforced by the simple-dwell comparator, which
also records zero switches in the ambiguous-tie scenario; the two rules differ
under transient, reversing and intermittent advantage.

The corrected decision-stream observer uses log-odds and requires 0.30 s of
observed follow-up. The `-class correspondence` CSV identifier denotes the
harness's synthetic label-flicker stress comparator, not an isolated removal from
the full policy. Offline transition blocking is not physical freezing or goal
failure. Historical core latency is not a latency measurement of the new Nav2 path.

The manuscript distinguishes the archived experiment configuration from the
opt-in responsive profile, measured-progress input, and candidate-trajectory
implementation. Closed-loop Gazebo/robot outcomes, independent
warranted-switch/free-motion oracles, external baselines (including a
previous-class consistency-weight baseline), and human motion-legibility studies
remain pending.

## Reproduction and source of truth

- [Current raw results and provenance](../../src/compass_eval/results/README.md)
- [Build/check the PDF](../../scripts/build_paper.sh)
- [Repository consistency gate](../../scripts/check_repo_consistency.py)
- [Merge-readiness checkpoint](../../docs/COMPASS_COMPLETION.md)

`artifact_manifest.json` records the source, PDF and figure hashes for the checked
snapshot. It is not a substitute for numerical tests or visual PDF inspection.
The [Korean draft](../paper_draft.md) is a companion explanation; the complete
formal statements and bibliography are authoritative in the English LaTeX.
