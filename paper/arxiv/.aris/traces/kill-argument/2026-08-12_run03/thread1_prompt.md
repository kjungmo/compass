You are simulating a hostile NeurIPS / ICRA / T-RO reviewer for a paper.
This is a kill-argument adversarial check — your task is NOT to give a
balanced review but to construct the **single strongest argument for
rejecting this paper**.

## Files to read
- LaTeX entry: main.tex (single-file paper, bibliography inline)
- Compiled PDF: main.pdf (compiled from the current main.tex, 36 pages)
- Figures: figures/*.pdf

Read the source carefully. Do not consult any prior reviews, fix lists,
or summaries; this must be a fresh, zero-context adversarial pass.

## Your task
Construct the single best argument to reject this paper in approximately
200 words. Your goal is to write the worst-case rejection memo a senior
area chair would produce after reading the paper.

Focus on these axes (pick the most damaging combination, do not list all):
1. Theorem validity: are central propositions actually proved as stated?
2. Assumption-vs-claim mismatch: does the body silently retreat to a
   narrower object than the title/abstract advertise?
3. Missing proof obligations: is a fundamental lemma invoked but not
   proved that the headline depends on?
4. Claim-vs-evidence gap: is the empirical/numerical evidence too narrow
   to support the breadth of the stated claims?
5. Scope overclaim: does the title or abstract sell a result substantially
   broader than what the body proves?
6. Novelty: is the stated gap actually open, given the related work the
   paper itself cites?

## Constraints
- Approximately 200 words total (do NOT exceed 250).
- Single argument, not a list — pick the most damaging line of attack
  and develop it.
- Cite specific file:line locations or equation numbers when accusing.
- Tone: dispassionate but uncompromising. Do NOT hedge. Do NOT acknowledge
  mitigations the paper might have made elsewhere. This is the rejection
  paragraph; the defense gets the next pass.
- Do NOT reference prior review rounds, fix lists, or any context outside
  the current paper files.

Output: just the rejection memo, nothing else.
