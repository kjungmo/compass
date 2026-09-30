# COMPASS main integration handoff

## What will merge

PR #15 (`review/completion-gates`) is the single integrated candidate targeting
`main`. The preparation baseline is main `9fe495a7f12e06cd2a3da46d0a3aa7f1db92859f`.
The original remote PR #15 head `1f17ba42` was 13 commits ahead and zero behind
that main, with 67 changed paths before this repository-wide cleanup.

The candidate contains the inherited #1 -> #2 -> #14 stack and the observer/theorem
repairs reviewed on #12/#13, followed by their additional corrections. A merge of
the final PR #15 tree updates the affected source, configuration, evaluation,
paper, figures, metadata, and CI files together. It does not automatically import
unrelated branches or turn unexecuted experiments into results. The English
Markdown entry points to canonical LaTeX/PDF instead of retaining a stale second
manuscript; the Korean companion explains its authority boundary.

Do not separately merge overlapping component PRs after the integration merge:
#1/#2/#14 are included by ancestry; #12/#13 are integrated with follow-up fixes,
not promised to be byte-identical to the earlier side branches. Their branches
remain preserved. Closing superseded PRs is a separate housekeeping decision.

## Review and verification contract

The initial green CI at `1f17ba42` was not whole-repository acceptance. It missed
stale paper prose/duplicates, unchecked numeric scripts, and software defects.
The multi-investigator review records are under `docs/reviews/merge-*.md`.
They are AI-agent reviews, not human approvals or independent empirical evidence.
Fresh reviewers must assess the repaired exact candidate before readiness is
declared; a clean thread count is not a review.

The final independent dispositions, corrected findings and exact-source evidence
are recorded in [reviews/final-integration.md](reviews/final-integration.md).
They cover the repaired integration, not just the original stacked PR delta.

Required gates for this research merge:

- Current main is an ancestor of the final candidate; no unreviewed incoming
  main change, conflict, or new PR head is silently discarded.
- Standalone core/trace/observer, response-profile, progress/candidate/safety,
  opportunity and physical-scorer boundary tests pass.
- CSV-to-table row identity, known stale prose regressions, package versions,
  local documentation links, figure copies and source/PDF hashes pass checks.
- Canonical PDF rebuilds with resolved references; changed pages are visually
  inspected. Hash correspondence does not alone prove PDF/source equivalence.
- ROS Jazzy builds all five packages and runs registered tests. Simulation
  installation/syntax checking is not Gazebo execution.
- Final PR CI is green and final review has no unresolved research-merge blocker.
  The PR is non-draft and targets main; main itself remains untouched here.

## Round-5 issue disposition boundary

| Issue | Internal merge-preparation work | What is not closed by it |
|---|---|---|
| #3 observer | Log-odds repair, positive observed hold, consistent tables/prose and version provenance | Human/motion legibility validity |
| #4 safety output | Explicit zero/small cap, dt-scaled braking and output regression | Physical stopping/collision guarantee |
| #5 P5 | Once-per-time rolling intervention count, absorbing HOLD/release, consistent pseudocode | Eventual progress or global deadlock escape |
| #6 P2 | Fixed/variable-period claims and finite-domain validation/tests | Physical timing bound without measured timing assumptions |
| #7 response | Persistent-selected-challenger conditions and clip/equilibrium boundary checks | Application-level warranted responsiveness |
| #8 progress | Signed measured seam, class/reset epoch tests and all-mode frame checks | General multi-person/Frenet ground-truth progress or validated 480-case trial |
| #9 attribution | Synthetic comparator labeling, bounded rollout cost seam and scoped claims | Isolated correspondence ablation, general homotopy realization |
| #10 observability | Offline traces, external-opportunity and physical-metric scorers/protocol | Integrated robot logging, independently annotated oracle and physical trials |
| #11 paper closure | Canonical manuscript/artifacts, repository checks, fresh AI review | Publication acceptance or an independent human review |

These are dispositions, not automatic issue closure instructions. Issues whose
physical/research conditions remain pending should stay open or be explicitly
split when the maintainer chooses to update the issue tracker.

## Separate decisions after preparation

For the maintainer's subsequent merge decision:

1. Open PR #15 and confirm its base is `main`, its reviewed head has not changed,
   and all three current-head jobs (standalone, paper, ROS Jazzy) are green.
2. Recheck that main is still the recorded baseline (or review any new incoming
   changes), there is no conflict, and any current branch protection, requested
   changes or approval requirement is satisfied. Never bypass a new rule.
3. Merge the integrated PR once. A merge commit preserves the inherited stack
   ancestry. This preparation does not execute that action or enable auto-merge.
4. The merged tree contains the affected implementation, tests, configs, current
   evaluation outputs, canonical LaTeX/PDF/figures, companion documents, package
   metadata and CI together. Unchanged files remain unchanged. Source packages
   must be rebuilt; merging is not installation or deployment.
5. Reconcile superseded component PRs separately, without replaying overlapping
   patches. Preserve their branches unless the maintainer authorizes cleanup.

The workflow also runs on subsequent pushes/merges to main. A green pre-merge
run does not replace checking that post-merge run, and a green post-merge run
does not establish physical performance.

Source packages are versioned 0.2.0 to signal changed C++ layouts/interfaces;
all consumers must rebuild. This is not a release publication. Defaults preserve
the legacy cost/progress profile; new paths are opt-in research implementations.
Physical freezing, goal success, collision/social-distance outcomes, external
baselines, human studies, and an independent warranted-switch/free-motion oracle
remain unmeasured. The archived latency is not a WCET/latency measurement of the
new paths. Keep original measurements available through their pinned Git history.

No email, merge to main, deployment, branch deletion or release is performed by
this preparation. At the initial inspection, main was unprotected and no rulesets
were configured; that is not a substitute for review and should be rechecked at
the actual merge decision. Do not bypass newly configured protection or approvals.
