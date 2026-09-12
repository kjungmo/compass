# Round 5 formal-claim correction

Stacked on the observer correction, so the regenerated data remains intact.
This change edits the English manuscript and rebuilds its PDF. Algorithm,
state and parameter values are unchanged.

- P2 explicitly distinguishes stored post-reset from pre-test evidence, handles
  the lower reflecting clip, gives variable-period accumulated-time separation,
  and includes the finite-window boundary term. It does not imply legibility.
- The response theorem requires persistent selected challenger, eligible
  updates, no interfering resets/HOLD, clip headroom and strict equilibrium
  headroom. Its converse remains valid for negative input bounds.
- Default rho > sqrt(2/3) blocks discretionary switches through e_max=.50,
  independently of noise. The leak-only .41 threshold cannot undo clipping.
- P5 is conditional window escalation to HOLD, not unconditional termination
  or collision freedom. The existing lifetime/window counter mismatch is
  explicitly retained as an open implementation gap.
- Supporting P1 leaky hitting-time and P4 integer-ceiling text is corrected.

The Korean historical theory draft is not synchronized by this English-theorem
change. Its evaluation tables were synchronized by the preceding observer
change. A fresh independent adversarial review on the resulting SHA is still
required; compiling and numerical checks are not a PASS verdict.

PDF built with pdfLaTeX/TeX Live 2023, microtype expansion disabled at build
invocation, with the same temporary CTAN dependencies as the observer build.
