# arXiv Submission Checklist — COMPASS

Source package: `paper/arxiv/main.tex` + `paper/arxiv/figures/*.pdf`.
Compiled with `tectonic main.tex` (zero errors; a handful of `Overfull \hbox`
warnings of at most ~0.5pt–35pt, i.e. sub-millimeter to ~1cm line overruns
from a few long inline `\texttt{...}` code paths — not visually disruptive,
acceptable for arXiv). 30 pages, 4 figures, 2 tables, 1 algorithm float,
28 references.

## 1. Category
- Primary: **cs.RO** (Robotics).
- Optional cross-list to consider: cs.MA (if emphasizing multi-agent
  crowd interaction) or cs.HC (for the legibility/HRI angle) — primary
  cs.RO is sufficient and most accurate; add cross-lists only if you want
  extra visibility, not required.

## 2. What to upload
Upload the **LaTeX source**, not just a compiled PDF:
- `main.tex`
- `figures/fig_class_lifecycle.pdf`
- `figures/fig_decision_flow.pdf`
- `figures/fig_oscillation_compare.pdf`
- `figures/fig_social_cost_field.pdf`

arXiv recompiles from source (it runs its own TeX toolchain), so do **not**
submit only `main.pdf`. Zip these five files together (flat structure, keep
the `figures/` subdirectory) and upload the zip, or upload the files
individually preserving the `figures/` path. Do not include `paper_en.md`,
`SUBMISSION.md`, or any `.aux`/`.log`/build artifacts in the upload.

Since figures are already PDF (vector, from matplotlib), no raster
conversion or resolution concerns — arXiv's TeX Live handles
`\includegraphics{.pdf}` natively via pdfTeX/XeTeX equivalents.

## 3. License recommendation
- If you may also submit to a venue (workshop/conference/journal) later,
  select **arXiv's non-exclusive perpetual license (arXiv 1.0)** at
  submission time — this preserves your ability to transfer copyright or
  grant an exclusive license to a publisher afterward without conflict.
- Only choose CC-BY-4.0 (or another CC license) if you are certain no
  venue with an incompatible copyright policy is in your near-term plans;
  once selected, the CC grant is generally treated as irrevocable for that
  version.
- Given the current status (decision-layer work with a Planned Evaluation
  section still open, likely to target a venue after R2/R4 measurements
  land), the arXiv 1.0 non-exclusive license is the safer default.

## 4. Comments field (suggested text)
```
28 pages (main.tex compiles to 30 with figures/tables/algorithm/references),
4 figures, 2 tables. Decision-layer design + formal properties (P1-P5) with
offline empirical validation (R1, R3, R5, measured); physical-simulation
metrics, external baselines (R2), and user study (R4) are specified as a
pre-registered Planned Evaluation (Sec. 5.6), not yet measured. Open-source
implementation: https://github.com/kjungmo/compass
```
Adjust the page count phrase to match whatever `main.pdf` reports at
final submission time (recompute after any last-minute edits).

## 5. Endorsement reminder
If this is the author's first submission to cs.RO on arXiv, an
**endorsement** from an existing arXiv author in cs.RO is required before
the paper can be posted. Request this *before* starting the submission
flow (arXiv emails an endorsement code request to the author's contacts /
suggested endorsers) — it can take 1–2 days. Check
https://arxiv.org/auth/endorse for current status before submitting.

## 6. AI-disclosure
Already included in the paper body (Acknowledgments section, end of the
Conclusion): *"AI tools assisted with translation from the Korean
manuscript and with editing; the author verified all technical content and
takes full responsibility."* No further action needed — this satisfies
arXiv's AI-use disclosure expectations. Do not add it a second time
elsewhere (e.g., as a footnote or in comments) to avoid duplication.

## 7. Pre-flight checks before clicking submit
- [ ] Recompile locally one more time from a clean directory
      (`rm -f *.aux *.log *.out; tectonic main.tex`) to confirm the
      uploaded source is self-contained and reproducible.
- [ ] Confirm all four figure PDFs are present under `figures/` in the
      upload and referenced paths match (`\includegraphics{figures/...}`).
- [ ] Double-check the abstract text pasted into the arXiv web form
      matches `main.tex`'s `\begin{abstract}...\end{abstract}` verbatim
      (arXiv requires the abstract as plain text in the form, separate
      from the PDF).
- [ ] Verify the GitHub link in Acknowledgments
      (`https://github.com/kjungmo/compass`) is public and live at
      submission time.
- [ ] Confirm full English text throughout (required since 2026-02-11;
      this package is already fully English — no Korean strings remain
      in `main.tex`).
