# Final independent integration review

## Reviewed source

Candidate: `54dfc43f85c2e91d1832be5e22ab69cf151e7c62`.
Tree: `9f541eb9e643933fd849dbb2d9ef7706724f507b`.
Base main: `9fe495a7f12e06cd2a3da46d0a3aa7f1db92859f`.

Six AI investigators participated: four initial investigators covered code,
paper, documentation and consistency gates; two fresh investigators independently
reviewed the repaired code and paper without implementing them. The consistency
investigator also cross-reviewed the coordinator's CI/build/integration work.
These are AI reviews, not human approvals or independent experimental evidence.

The fresh reviews first covered local code snapshot `c9337951` and were carried
to published `cc71dcb4` after verifying identical runtime source and only
manuscript layout/PDF/manifest differences. The final `54dfc43f` changes only a
stale version-test expectation and its consistency/mutation checks. The code
and integration investigators independently reviewed that narrow delta; paper
source and artifacts are unchanged. Publication used the connected GitHub API;
the full remote tree hashes were compared with the locally tested trees.

## Independent findings and resolution

| Investigator | Concrete final-pass finding | Verified resolution |
|---|---|---|
| Fresh code | Asymmetric covariance `{1,100,-100,1}` was averaged into an apparently valid matrix and finite candidate cost | Exact-source probe changes from finite `J=1.04756` to unavailable `J=inf`; five symmetry/tolerance/legacy regression fixtures |
| Fresh paper | Blanket lockout at reachable upper bound `<= E0` incorrectly included attainable clip equality | Strict inequality and explicit clip/equilibrium equality distinction; no theorem/numeric change hidden as formatting |
| Fresh paper | Remaining paragraph claimed safe-set-only complexity and treated measured batch maximum as a timing upper bound | All-candidate/all-people dependency and historical finite-batch/non-WCET scope stated in both manuscript languages |
| Paper visual cross-check | Interface table was unreadably scaled; reproduction paths clipped at page edges | Wrapped two-column table and breakable/shortened paths; final 40-page PDF inspected, including pages 14, 27, 32 and 39–40 |
| Native CI | Smoke test still expected `0.1.0` after the documented `0.2.0` interface bump | Expectation corrected, not removed; standalone gate now rejects stale or absent version assertions |

Earlier substantive safety, state, evidence and documentation findings and their
reproductions remain in [merge-code.md](merge-code.md),
[merge-paper.md](merge-paper.md), [merge-docs.md](merge-docs.md) and
[merge-consistency.md](merge-consistency.md).

The three independent review submissions are visible on PR #15:

- [Code review](https://github.com/kjungmo/compass/pull/15#pullrequestreview-5188254728)
- [Paper review](https://github.com/kjungmo/compass/pull/15#pullrequestreview-5188254755)
- [Integration review](https://github.com/kjungmo/compass/pull/15#pullrequestreview-5188254784)

Their COMMENT state is deliberate; none is represented as a human APPROVED review.

## Verification evidence

Local verification passed for core trace/state equivalence (50,000 cycles),
1,500 current CSV rows, response profiles/bounds, progress/candidate/safety
contracts, opportunity and physical-scorer tests, every checked paper-table cell,
source/PDF/figure hashes, local links, package/runtime/smoke versions, and 15
consistency mutation tests (also under Python optimization).

The coordinator rebuilt the canonical PDF with three pdfLaTeX passes using TeX
Live 2023 and cached algorithm/algorithmicx packages, then rendered and inspected
the affected pages. One fresh investigator's unprovisioned local TeX environment
lacked `algorithm.sty`; that investigator's own rebuild is not claimed successful.
The separately provisioned paper CI build is the independent compilation gate.

[Run 34721644725](https://github.com/kjungmo/compass/actions/runs/34721644725)
at `cc71dcb4` passed standalone and paper jobs and built all five ROS packages.
Its sole failing fixture was the stale smoke-version assertion; colcon reported
it twice through CTest/gtest aggregation. This run is not described as green.
The subsequent exact-candidate run
[34721916869](https://github.com/kjungmo/compass/actions/runs/34721916869)
passed all three jobs: standalone, paper and ROS Jazzy. All five packages built;
all registered tests passed (colcon reports 73 aggregated entries, zero errors,
failures or skips). Simulation launch/world/executable installation and Python
syntax checks passed separately. No Gazebo process was executed by these checks.

The final handoff commit adds/updates review and checkpoint documents only.
Runtime, manuscript, artifact manifest, tests and workflow remain exactly those
of the verified candidate above. Its own current-head CI must also be green;
the live checks on PR #15 record that terminal result without an endless series
of additional documentation-only commits just to cite themselves.

## Disposition boundary

No unresolved software/manuscript integration blocker was identified by the
final investigators within the documented research scope. Current-head CI and
the final PR state remain the merge gate; an empty review-thread count alone
is not evidence of review. A later code/paper/configuration change requires a
new scoped verification decision, not automatic reuse of this disposition.

Main merge, deployment, release and branch deletion have not been performed.
Gazebo/robot trials, physical stopping/freezing/goal success, independently
annotated opportunity/free-motion oracles, external baselines, human legibility
and human peer review remain separate. The simulation-package build/asset check
does not execute Gazebo. See [../MERGE_READINESS.md](../MERGE_READINESS.md).
