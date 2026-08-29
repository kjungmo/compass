### Point P_1: Headline–theorem mismatch

**Attack claim**: The paper intentionally treats P2’s inter-switch lower bound as its deterministic anti-oscillation guarantee, although that bound may add nothing beyond the sampling interval.

**Verdict**: partially_answered

**Evidence (or lack of)**: The paper explicitly adopts this narrow interpretation in the abstract and formal scope ([main.tex:38](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:38), [main.tex:294](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:294)). P2 bounds only \(\Delta t_{\mathrm{switch}}\) ([main.tex:390–395](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:390)); nowhere is the bound required to exceed one control period. This position is intentional but unsustainable as substantive anti-oscillation when the bound is scheduler-trivial.

**Severity if unresolved**: critical

**If unresolved, recommended fix**: Rename P2 as a parameter-dependent rate inequality unless the paper adds a declared, nontrivial minimum-cycle condition.

### Point P_2: Admissible one-cycle counterexample

**Attack claim**: The paper’s own admissible parameter witness permits a maximal innovation to cross the threshold in one cycle, allowing discretionary class flips every sampled cycle.

**Verdict**: still_unresolved

**Evidence (or lack of)**: The illustrative boundary witness—not the deployment default—is \(dt=.05\), \(\Delta_{\mathrm{floor}}=.05\), \(D_{\max}=.95\), \(E_0=.03\), and \(e_{\max}=.045\), and is explicitly said to satisfy the two-sided condition ([main.tex:439–440](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:439)). Recalculation gives
\[
\frac{.03}{.95-.05}=.0333\text{ s}<.05\text{ s},\qquad
(.95-.05)(.05)=.045>.03.
\]
After each reset ([main.tex:248](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:248)), an alternating maximal advantage can therefore switch every cycle. At \(\rho=0\), the \(0.05\)-s interval strictly exceeds—not equals—the \(0.0333\)-s bound; equality occurs at \(\rho=.5\), where \(E_{\mathrm{th}}=.045\). Either way, P2 permits sample-rate switching. Deployment defaults differ ([main.tex:759–764](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:759)), but no default \(D_{\max}\) is printed, so their P2 bound cannot be fully recomputed.

**Severity if unresolved**: critical

**If unresolved, recommended fix**: State
\[
N_{\min}=\left\lceil\frac{E_0}{(D_{\max}-\Delta_{\mathrm{floor}})dt}\right\rceil
\]
and require a justified \(N_{\min}\ge2\), then test the alternating-maximal-advantage sequence.

### Point P_3: P2 vacuity versus liveness

**Attack claim**: A policy that never switches satisfies P2, so the headline theorem itself cannot distinguish bounded switching from permanent lockup.

**Verdict**: partially_answered

**Evidence (or lack of)**: The text explicitly recognizes that clipping may block switching while P2 remains true ([main.tex:395](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:395)), calls \(\bar e^{\mathrm{rev}}_{\max}\le E_0\) an inert, meaningless regime ([main.tex:432](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:432)), and separately imposes \(\bar e^{\mathrm{rev}}_{\max}>E_0\) as liveness at \(\rho=0\) ([main.tex:506–510](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:506)). This distinguishes parameter-level reachability from rate control, but gives no eventual or timely-response guarantee and is not a premise of P2.

**Severity if unresolved**: major

**If unresolved, recommended fix**: Add a liveness proposition giving a finite response-time upper bound under a stated sustained-advantage condition, including progress hardening.

### Point P_4: Missed five-margin reversal

**Attack claim**: Under the default knobs, COMPASS retains its initial class even when the late challenger advantage reaches \(0.25\), five times the \(0.05\) margin.

**Verdict**: partially_answered

**Evidence (or lack of)**: The table reports zero switches for COMPASS in `mid_reversal` ([main.tex:564–566](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:564)); the discussion verifies \(0.25=5\Delta_{\mathrm{floor}}\), calls the result a masking effect, and says the accumulator “fails to track” the reversal ([main.tex:586](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:586)). Late locking is intentional ([main.tex:407](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:407)), but is not sustainable without a criterion separating warranted reversals from deliberately rejected ones.

**Severity if unresolved**: major

**If unresolved, recommended fix**: Label warranted-switch intervals and report switch recall and response delay, with an explicit rule defining when point-of-no-return locking is acceptable.

### Point P_5: Progress-rate freezing region

**Attack claim**: Progress hardening blocks every justified switch at \(v_{\mathrm{lat}}\ge0.35\) m/s, revealing a broad lockout region rather than responsive consistency.

**Verdict**: partially_answered

**Evidence (or lack of)**: The paper reports \(0/50\) switches at \(v_{\mathrm{lat}}\ge0.35\) m/s and identifies the cause as approximating cross-track progress with forward speed ([main.tex:588](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:588)). The configured cruise speed is \(0.45\) m/s ([main.tex:778](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:778)), although actual encounter speed is not reported. The defect is acknowledged only as grounds for future correction.

**Severity if unresolved**: critical

**If unresolved, recommended fix**: Implement \(\rho\) from realized cross-track displacement and demonstrate warranted-switch liveness across the full deployed speed range.

### Point P_6: Decision-stream metrics reward freezing

**Attack claim**: Decision-only measurements can assign favorable switch-count, entropy, and proxy-legibility scores to a stable but wrong or frozen commitment.

**Verdict**: partially_answered

**Evidence (or lack of)**: The scope is intentional and transparent: all figures derive from the decision stream ([main.tex:520](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:520)); offline legibility substitutes committed-side decisions for realized motion ([main.tex:524](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:524)); and trajectory metrics remain unmeasured ([main.tex:530–531](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:530)). The paper even says its proxy can mask switching failure ([main.tex:586](/home/cona/kangj/compass-aris-pilot/paper/arxiv/main.tex:586)). This scope is sustainable only for “internal stream stability,” not responsiveness, motion legibility, or anti-freezing.

**Severity if unresolved**: major

**If unresolved, recommended fix**: Add realized-trajectory freezing metrics or, minimally, warranted-switch recall, response delay, and lockup rate to the current harness.

## Summary

Total rejection points: 6

- answered_by_current_text: 0
- partially_answered: 5
- still_unresolved: 1

## Net assessment

The paper would probably not survive a senior area-chair reading of this attack. It candidly discloses the liveness, proxy, reversal, and freezing limitations, but those disclosures do not repair the headline mathematics: P2’s own formally admissible witness allows cycle-rate discretionary switching, so the advertised deterministic anti-oscillation interpretation is unsupported.

## Top action items (in priority order, max 3)

1. Replace P2 with an exact discrete-cycle bound plus an explicit, nontrivial multi-cycle requirement.
2. Add a complementary finite-response liveness theorem and repair the forward-speed-based progress estimator.
3. Evaluate warranted-switch correctness, response delay, freezing, and realized motion—not switch suppression alone.
