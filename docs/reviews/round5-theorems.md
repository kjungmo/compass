# Round 5 formal-claim correction

Stacked on the observer correction, so the regenerated data remains intact.
This change edits the English manuscript and aligns the P5 state contract in
the implementation. Other parameter values are unchanged.

- P2 explicitly distinguishes stored post-reset from pre-test evidence, handles
  the lower reflecting clip, gives variable-period accumulated-time separation,
  and includes the finite-window boundary term. It does not imply legibility.
- The response theorem requires persistent selected challenger, eligible
  updates, no interfering resets/HOLD, clip headroom and strict equilibrium
  headroom. Its converse remains valid for negative input bounds.
- Default rho > sqrt(2/3) blocks discretionary switches through e_max=.50,
  independently of noise. The leak-only .41 threshold cannot undo clipping.
- P5 is conditional window escalation to HOLD, not unconditional termination
  or collision freedom. The implementation now counts the open-left rolling
  window, records at most once per decision time, returns zero in HOLD, and
  requires an explicit release call.
- Supporting P1 leaky hitting-time and P4 integer-ceiling text is corrected.

The Korean draft's P5 and empirical-scope wording is synchronized; the English
manuscript remains the formal theorem target. A fresh independent adversarial
review on the resulting SHA is still required; compiling and numerical checks
are not a PASS verdict.

The integrated 40-page PDF was rebuilt with the cached algorithm/algorithmicx
packages and visually checked on the changed theorem and result pages.
