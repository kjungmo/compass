# Main-merge documentation/build audit

Reviewer: an independent AI investigator, initially read-only; subsequently
assigned the documentation remediation below. This is not external human peer
review or a deployment approval. Initial source: local `9bd25aa`, whose tracked
tree exactly matched remote PR #15 `1f17ba42`; comparison base: main `9fe495a`.

## Initial findings and bounded remediation

1. **P1 — active public claims were inconsistent with the reviewed evidence.**
   Root README called R5 a physical "freezing threshold", treated P5 as general
   deadlock escape, described a 33-page arXiv-ready paper, and presented archived
   latency without separating the new execution path. Results README retained
   incorrect censoring counts. Root/results README now distinguish offline
   transition blocking, conditional finite-window HOLD, draft status, historical
   latency and corrected 72/250 and 71/250 censoring. Final-source physical
   safety/freezing/success/human-legibility validation remains pending.
2. **P1 — competing English manuscript silently retained refuted claims.**
   `paper/arxiv/paper_en.md` still asserted old P1/P5 statements and a false
   intermittent full-method 50/50 censoring result. Remediation is assigned to
   the integration owner: replace it with a canonical `main.tex`/PDF entrypoint
   or faithfully synchronize it. The manuscript specialist independently owns
   the authoritative LaTeX/PDF and Korean critical-section review.
3. **P2 — source and integration documentation described obsolete stages.**
   `src/README.md` said implementation was empty; RESPONSIVENESS said ROS progress
   was not wired. Package notes now describe the actual packages, independent
   opt-ins, configure-time parameters, estimator limits and mandatory rebuilds.
   OBSERVABILITY/RESPONSIVENESS/candidate docs distinguish current corrected CSV
   regression from the original Git archive and trace/no-trace from old/new
   policy equivalence. Historical validation transcripts are not final CI proof.
4. **P2 — fresh-workspace simulator build and provenance were misleading.**
   Simulator instructions selected packages but omitted the local message
   dependency. They now use `--packages-up-to compass_sim` over `src sim`.
   Old smoke observations are explicitly historical, lack retained source/config
   hashes and logs, and were not rerun on the current optional paths. Server-only
   `-s` does not disable GPU sensor rendering; a GL-version override is not a
   software-renderer selector. Package manifest/CI dependency fixes are assigned
   to the integration owner.
5. **P2 — CI did not gate the complete documented software evidence.**
   Initial CI omitted the three paper/observer/recurrence checkers and full
   response sweep, and its ROS build omitted `sim`. The integration owner owns
   the added gates. A simulator package build must remain distinct from Gazebo
   runtime/physical validation. Final combined source needs its own passing run.
6. **P2 — ABI-changing public interfaces retained version 0.1.0.**
   The integration owner owns manifest/source-version updates to 0.2.0; docs and
   CITATION now require all dependent binaries to rebuild and explicitly avoid
   calling this a published release or binary-compatible patch.
7. **P1 — cross-review found unsafe default-mode environment behavior.**
   `src/compass_nav2/src/costmap_env_query.cpp` computed legacy TTC with the sign
   of robot-minus-person closing velocity reversed: a stationary person ahead of
   an approaching robot produced the large fallback TTC. Legacy occupancy also
   treated absent maps, unknown cells and out-of-map queries as free. These are
   not acceptable merely because candidate mode is opt-in; the integration owner
   assigned code corrections and regression cases to the controller/core reviewer.
   Default parameter values and the offline archived battery can be retained
   while intentionally correcting unsafe runtime behavior. Final acceptance
   requires those corrections and tests, not a limitation-only disclaimer. The
   stable revised source was cross-read: it corrects the TTC sign and L/R bias,
   fails closed for absent/unobserved maps and all-mode frame mismatch, and
   suppresses immediately blocked legacy commands. Documentation now records
   these intentional runtime changes and retains the proxy/physical boundary.

## Checks performed on the initial tree

- `git diff --exit-code HEAD origin/review/completion-gates`: identical trees.
- `git merge-base --is-ancestor main HEAD`: main is an ancestor of the full stack.
- `python3 scripts/check_observer_results.py`: passed against CSV, R1, Korean
  draft and English LaTeX; this did not cover the stale READMEs/English Markdown.
- `python3 scripts/check_switch_bounds.py`: passed numerical falsification checks,
  not a mathematical proof or robot test.
- `python3 scripts/check_paper_numbers.py`: passed its bounded Korean numeric
  checks, not all prose/source consistency.
- All four `paper/arxiv/figures/*.pdf` copies matched their `paper/figures/`
  counterparts byte-for-byte. The oscillation figure generator reads switch
  counts from CSV; those metrics did not change with the observer correction.
- `pdfinfo paper/arxiv/main.pdf`: the initial reviewed PDF had 40 pages, not 33.

After the owned documentation edits, the new consistency gate's active README
scope, labeled result-summary counts, repository-local Markdown targets, and
package-version checks passed. `git diff --check` passed. These checks do not
replace final-source CI or the manuscript investigator's source/PDF review.

The final frozen LaTeX was cross-read against the active READMEs and integration
documents. That re-review corrected the remaining intended-lifecycle versus
input-order top-K distinction, the deterministic P2/probabilistic P1 mapping,
fresh-workspace reproduction instructions, observer-version distinction and
universal safety-commit progress reset. The results notes also explain the
initial-output counting boundary and unclipped Gaussian-input theorem boundary.
No remaining active-document/source mismatch was found in this bounded review.
After the coordinator generated the final PDF/manifest, this investigator reran
the complete repository-consistency, strengthened observer-table and paper-number
checks: all passed, as did `git diff --check`. PDF text extraction independently
confirmed the 40-page artifact contains the input-order distinction, conditional
HOLD, 72/250 and 71/250 counts, and separate 0.30 s metric-definition change.
The coordinator owns visual PDF QA and exact-head ROS/PR CI; these local checks
do not certify either or authorize a main merge.

## Merge and external evidence boundary

At the initial snapshot, PR #15 targeted PR #14's branch rather than main. Its
full history contains the earlier paper-hardening, observability and responsiveness
stack; merging into that original base alone would not deliver everything to main.
During remediation the integration owner retargeted PR #15 to main, making it a
full-stack main-directed review. Final readiness requires verifying that exact
diff preserves the integrated observer/theorem content and passes CI against main.
Independent investigators must re-review the combined edits; a green earlier head
is not enough. This task authorizes review and preparation, not the actual main
merge. Merging, robot deployment, physical experiments and unrelated communication
remain separate decisions; this investigator performed no remote writes.

The pending 480-case pilot is a plan, not results: fixtures, independent actor
and opportunity/free-motion oracles, controller adapter and source/config/input
provenance must be bound before running or aggregating trials. No research
claim may convert a missing case to success or count it in a measured denominator.
