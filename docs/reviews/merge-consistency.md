# Merge consistency gate

This is a bounded mechanical check of the current six-variant, five-scenario
research snapshot. Run from the repository root:

```bash
python3 scripts/check_repo_consistency.py
python3 scripts/test_repo_consistency.py
python3 -O scripts/check_observer_results.py
python3 scripts/check_paper_numbers.py
```

The checker is read-only, uses the Python standard library (Python 3.9+), exits
nonzero on a mismatch, and prints explicit errors. Its conditions do not use
Python `assert`, so optimization with `python3 -O` does not remove the gate.

## What is checked

- The raw ablation CSV contains exactly the declared 1,500 unique trials: six
  recognized variants, five recognized scenarios, and seeds 0–49 in each cell.
  Metric values must be finite and nonnegative; censor flags must be 0 or 1.
- Every aggregate row in R1, the canonical English LaTeX manuscript, and the
  non-canonical Korean draft is mapped by its variant label, not row order.
  Every mean and sample SD for switches, sign-change rate, entropy, and
  time-to-legible is recomputed from that variant's CSV group at the published
  two-/three-decimal precision. The censor numerator and denominator must match
  that same group exactly. A number elsewhere in a table or paragraph cannot
  satisfy the check. The corresponding published per-scenario switch tables and
  R1's per-scenario observer means/censor counts are also checked cell by cell.
- The labelled censor-count summary in the results README matches the CSV. It
  must include the accumulator and class-correspondence ablations; additional
  rows must also match their own variants. The retired `48/250` aggregate claim
  is rejected in the active manuscripts, R1, and results README. This is a
  targeted known-regression check, not arbitrary natural-language fact checking.
- `paper_en.md` identifies and links the canonical `main.tex` and `main.pdf`
  and does not reintroduce a full duplicate English abstract/chapter body. The
  Korean draft's opening declares its non-canonical role and links the source.
- Active README checks cover offline measurement scope, research-draft scope,
  the obsolete empty-source placeholder, historical simulation-smoke scope,
  and the known R5 physical-freezing overclaim. The checks recognize English or
  Korean scope terms; they do not enforce full sentences or future feature work.
- The five package manifests declare one version, and the decision core's
  `version()` reports it. There is no forever-fixed version or minimum bump.
- Simple relative Markdown links, image destinations, reference definitions,
  and HTML `href`/`src` paths resolve to existing repository files/directories.
  Heading fragments, remote URLs, absolute routes, fenced examples, and explicit
  revision-history sections/`git:REV:path` references are not checked. Hidden,
  build, install, log, and dependency trees are excluded. Link existence does
  not imply the destination says what the referring text claims.
- The artifact manifest lists exactly the canonical LaTeX source, its PDF,
  and the four figure files referenced by `includegraphics`. Every SHA-256
  matches current bytes; `main.pdf` has a PDF signature. Each of the four
  duplicated arXiv figure PDFs must be byte-identical to its `paper/figures/`
  counterpart.

`check_observer_results.py` remains a focused compatibility entry point for the
same stronger table checker; it no longer relies on positional zip, numeric
substring presence, or removable assertions. `check_paper_numbers.py` remains a
supplemental legacy citation-presence/retired-number check for R3/R5 and the
Korean draft. Its prose-number presence checks are weaker than the labelled
cell comparisons and should not be described as full semantic verification.

## Source/PDF manifest contract

The format of `paper/arxiv/artifact_manifest.json` is:

```json
{
  "schema_version": 1,
  "artifacts": {
    "paper/arxiv/main.tex": {"sha256": "64 lowercase hexadecimal characters"},
    "paper/arxiv/main.pdf": {"sha256": "64 lowercase hexadecimal characters"},
    "paper/arxiv/figures/fig_class_lifecycle.pdf": {"sha256": "..."},
    "paper/arxiv/figures/fig_decision_flow.pdf": {"sha256": "..."},
    "paper/arxiv/figures/fig_social_cost_field.pdf": {"sha256": "..."},
    "paper/arxiv/figures/fig_oscillation_compare.pdf": {"sha256": "..."}
  }
}
```

All keys are repository-relative paths. A changed source, output PDF, or figure
invalidates the recorded snapshot. Refresh the manifest only after building
and reviewing the matching PDF; do not bless an old PDF by merely rehashing
changed source. The manifest checker itself cannot establish that the PDF was
produced from the listed source, nor establish compiler provenance. A separate
paper-build check and human artifact review remain necessary.

## What passing does not prove

Passing does not validate the mathematical proofs, interpret every sentence,
independently recompute decision streams from raw trajectories, verify the
observer model scientifically, or establish robot safety or physical efficacy.
It does not substitute for core/property/ROS tests, replay/response regression,
PDF compilation and visual review, external-baseline comparison, Gazebo/robot
trials, or human legibility studies. Package version agreement is not an ABI
checker. The link scan is deliberately not a complete CommonMark parser or an
online URL checker. Four figure PDFs with matching bytes need not encode the
correct science or match their PNG previews; numerical figure-content review
remains separate.

The mutation suite checks correct current tables and intentionally wrong metric
cells, variant assignments, censor counts, row labels, README summary counts,
links, source/PDF/figure hashes, duplicated figures, and optimized-Python
failure behavior. New variants, scenarios, published tables, artifact types,
or link syntax require an explicit update to this bounded contract.
