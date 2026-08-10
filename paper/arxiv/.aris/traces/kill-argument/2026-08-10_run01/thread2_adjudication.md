The current artifacts materially disagree: [main.tex](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex) contains targeted amendments, while [main.pdf, pp. 14–15](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.pdf) still contains the attacked finite-variance-only proposition, absorbing-barrier language, and infinite-hitting-time argument. Verdicts below assess the newest LaTeX text; the stale PDF weighs against the net assessment. The figures add no missing stochastic assumptions.

### Point P1: Student-\(t_3\) counterexample

**Attack claim**: Centered i.i.d. Student-\(t_3\) satisfies every P1 assumption, giving a polynomial one-cycle crossing probability that contradicts the exponential bounds.

**Verdict**: answered_by_current_text

**Evidence (or lack of)**: P1 now explicitly requires \(|D_k|\le D_{\max}\) almost surely at [main.tex:281](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:281). The paper expressly identifies Student-\(t_3\) as the finite-variance counterexample and excludes it through bounded support at [main.tex:289](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:289). Thus \(t_3\) no longer satisfies every source-level assumption. The compiled PDF remains stale.

**Severity if unresolved**: critical

### Point P2: Stationarity still cannot prove P1

**Attack claim**: Negative mean drift—even with bounded innovations—does not establish the claimed exponential switching rate for an arbitrary stationary, temporally dependent process.

**Verdict**: still_unresolved

**Evidence (or lack of)**: P1 assumes only stationarity at [main.tex:281](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:281), but the proof silently invokes “weakly correlated” increments and Hoeffding at [main.tex:299](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:299), while [main.tex:318](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:318) admits standard results require IID structure. A bounded stationary latent-regime process \(D_k=Z\) can have one branch that never hits, defeating P1. The displayed coefficient remains only quasi-Gaussian.

**Severity if unresolved**: critical

**If unresolved, recommended fix**: State a precise IID, conditional-MGF, or quantitative geometric-mixing assumption in P1 and prove a regenerative or finite-horizon bound for the reflected recursion using an exact adjustment coefficient.

### Point P3: Reflecting boundary and eventual hitting

**Attack claim**: Zero clipping is reflecting rather than absorbing, so sufficiently positive i.i.d. innovations can produce almost-sure crossing and finite expected hitting time.

**Verdict**: answered_by_current_text

**Evidence (or lack of)**: The actual clipped update appears at [main.tex:176](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:176). The current proof explicitly calls zero reflecting and says positive increments leave it at [main.tex:293](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:293), then retracts the never-hit/infinite-mean argument at [main.tex:295](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:295). That directly answers the attack, although the replacement’s general recurrence claim still needs the stronger assumptions identified in P2. The PDF is unsynchronized.

**Severity if unresolved**: major

### Point P4: Remark 2 remains incomplete

**Attack claim**: Mixing alone does not create exponential tails, and an unspecified effective coefficient cannot serve as a theorem for autocorrelated advantages.

**Verdict**: partially_answered

**Evidence (or lack of)**: [main.tex:318](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:318) now correctly says mixing alone is insufficient and identifies bounded range as supplying exponential moments. However, it retains the “even otherwise” effective-coefficient fallback for arbitrary autocorrelation. Long-run variance does not determine a large-deviation rate, and no stated theorem connects it to this reflected process’s switching rate.

**Severity if unresolved**: critical

**If unresolved, recommended fix**: Delete the arbitrary-autocorrelation fallback and state one precise dependence condition with a proved exponential maximal or first-passage inequality and its actual constants.

### Point P5: Headline provable novelty

**Attack claim**: Because P1 fails, the advertised novelty of provable anti-oscillation no longer distinguishes the method from prior temporal-consistency mechanisms.

**Verdict**: partially_answered

**Evidence (or lack of)**: The categorical “all novelty is destroyed” claim is too broad: the deterministic P2 inter-switch bound remains at [main.tex:325](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:325) and [main.tex:330](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:330). Nevertheless, the gap claim still advertises “provable anti-oscillation” at [main.tex:78](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:78), repeated in the abstract and conclusion at [main.tex:38](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:38) and [main.tex:580](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:580), while P1 remains unproved under stationarity.

**Severity if unresolved**: critical

**If unresolved, recommended fix**: Restrict every headline guarantee to P2’s deterministic result unless P1 is restated and rigorously proved under explicit dependence assumptions.

### Point P6: Internal ablations cannot replace P1

**Attack claim**: Scripted offline internal ablations cannot validate an exponential stochastic guarantee over P1’s advertised process class.

**Verdict**: partially_answered

**Evidence (or lack of)**: These are genuine implementation measurements: [main.tex:451](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:451) uses the actual decision core, and scenario results at [main.tex:497](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:497) show strong internal separation. The paper also explicitly limits their scope at [main.tex:446](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:446). They support scenario-specific mechanism behavior but cannot establish P1; [main.tex:517](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:517) also reports adverse cases.

**Severity if unresolved**: major

**If unresolved, recommended fix**: Separate the empirical mechanism claim from P1 and add threshold-scaling first-passage stress tests across the precise stochastic class retained by the corrected theorem.

### Point P7: External and physical validation absent

**Attack claim**: No measured external-planner comparison or physical success, collision, social-distance, freezing, or real-robot navigation outcome supports the broader navigation claims.

**Verdict**: partially_answered

**Evidence (or lack of)**: This is an intentional, transparent scope decision: [main.tex:550](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:550) labels every physical, external, and user evaluation unmeasured; physical metrics remain TBD at [main.tex:556](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:556), and external results remain TBD at [main.tex:562](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:562). Moreover, the current harness excludes the safety-override collision regime at [main.tex:453](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:453). Preregistration is not performance evidence; this position is not sustainable once P1 remains unproved.

**Severity if unresolved**: major

**If unresolved, recommended fix**: Execute the preregistered independently tuned external-baseline and physical simulation or real-robot evaluation before making broad navigation or safety-effectiveness claims.

## Summary

Total rejection points: 7

- answered_by_current_text: 2
- partially_answered: 4
- still_unresolved: 1

## Net assessment

The paper would not survive a senior area-chair read in its current state. The newest LaTeX successfully excludes the literal Student-\(t_3\) counterexample and corrects the reflecting-boundary error, but P1 remains false under its stated arbitrary-stationarity premise, and Remark 2 does not close that gap. The compiled PDF still presents the original rejected theorem. The surviving P2 result and internal measurements do not sustain the headline probabilistic novelty, especially without external or physical validation.

## Top action items (in priority order, max 3)

1. Replace P1 with a rigorous theorem for the reflected recursion under explicit dependence, accessibility, and exponential-moment assumptions.
2. Remove unsupported effective-coefficient claims, align all headline language, enforce the bounded normalization operationally, and regenerate `main.pdf`.
3. Complete the preregistered external-planner and physical safety/navigation evaluation.

```json
{"points":[{"id":"P1","label":"Student-t3 counterexample","verdict":"answered_by_current_text","severity":"critical"},{"id":"P2","label":"Stationarity still cannot prove P1","verdict":"still_unresolved","severity":"critical"},{"id":"P3","label":"Reflecting boundary and eventual hitting","verdict":"answered_by_current_text","severity":"major"},{"id":"P4","label":"Remark 2 remains incomplete","verdict":"partially_answered","severity":"critical"},{"id":"P5","label":"Headline provable novelty","verdict":"partially_answered","severity":"critical"},{"id":"P6","label":"Internal ablations cannot replace P1","verdict":"partially_answered","severity":"major"},{"id":"P7","label":"External and physical validation absent","verdict":"partially_answered","severity":"major"}],"net":"does_not_survive"}
```
