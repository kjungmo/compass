I found no hallucinated works. Five entries need bibliographic corrections; one is clean. The main substantive concern is that the manuscript sometimes portrays Social Momentum as a stateful evidence accumulator, whereas the cited method recomputes angular momentum while preserving its current sign.

### `kirby2010thesis`

**EXISTENCE — YES.** The underlying thesis exists. [Official CMU record](https://publications.ri.cmu.edu/social-robot-navigation) · [Thesis PDF](https://publications.ri.cmu.edu/storage/publications/pub_files/2010/5/rk_thesis.pdf)

**METADATA**

- Authors: **correct** — Rachel Kirby, appropriately abbreviated R. Kirby.
- Year: **correct** — 2010.
- Venue/type: **correct** — PhD thesis, Carnegie Mellon University; technical-report number CMU-RI-TR-10-13.
- Title: **wrong: should be** *Social Robot Navigation*. “(asymmetric Gaussian proxemic cost)” is an explanatory annotation, not part of the title.
- Volume/pages: **correct/not applicable** — a thesis has no journal volume or conventional page range; the report number is correct.

**CONTEXT**

- `main.tex:64` — **SUPPORTS.** Kirby represents personal space and other social conventions as mathematical cost functions for navigation.
- `main.tex:139` — **SUPPORTS.** Appendix A explicitly defines the asymmetric Gaussian with different forward/head, side, and rear spreads.

**VERDICT — FIX.**

---

### `bhattacharya2012topological`

**EXISTENCE — YES.** [DBLP record](https://dblp.org/rec/journals/arobots/BhattacharyaLK12) · [Author manuscript](https://www.lehigh.edu/~sub216/local-files/topology_AURO_author_version_57596.pdf)

**METADATA**

- Authors: **correct** — Subhrajit Bhattacharya, Maxim Likhachev, Vijay Kumar.
- Year: **correct** — 2012.
- Venue: **correct** — *Autonomous Robots*.
- Title: **wrong: should be** *Topological Constraints in Search-based Robot Path Planning*. The H-signature/homotopy parenthetical is not part of the title.
- Volume/pages: **correct** — 33(3), 273–290.

**CONTEXT**

- `main.tex:70` — **SUPPORTS.** The paper uses H-signatures in graph search to obtain representatives from distinct homotopy classes. Strictly, it states that H-signatures are homology—not homotopy—invariants; it also includes known moving-obstacle examples, but not interactive moving pedestrians.

**VERDICT — FIX.**

---

### `dragan2013legibility`

**EXISTENCE — YES.** [DBLP record](https://dblp.org/rec/conf/hri/DraganLS13) · [Paper PDF](https://personalrobotics.cs.washington.edu/publications/dragan2013legibility.pdf)

**METADATA**

- Authors: **wrong: should be** A. D. Dragan, **K. C. T. Lee**, and S. S. Srinivasa. Lee’s `T.` initial is missing.
- Year: **correct** — 2013.
- Venue: **correct** — 8th ACM/IEEE International Conference on Human-Robot Interaction.
- Title: **correct** — *Legibility and Predictability of Robot Motion*.
- Volume/pages: **correct** — no volume; pp. 301–308.

**CONTEXT**

- `main.tex:76` — **SUPPORTS.** The work defines legible motion as motion that enables an observer to infer the robot’s intent; the proposed connection from reduced oscillation to legibility is the manuscript’s own extension.
- `main.tex:394` — **SUPPORTS.** Dragan et al. explicitly formulate legibility through observer inference from partial trajectories to goals. It supports that perspective, though not the manuscript’s particular lateral-trajectory likelihood model or its separate unbiasedness claim.

**VERDICT — FIX.**

---

### `trautman2010`

**EXISTENCE — YES.** [DBLP record](https://dblp.org/rec/conf/iros/TrautmanK10) · [IEEE DOI](https://doi.org/10.1109/IROS.2010.5654369)

**METADATA**

- Authors: **correct** — Peter Trautman and Andreas Krause.
- Year: **correct** — 2010.
- Venue: **correct** — IEEE/RSJ International Conference on Intelligent Robots and Systems.
- Title: **correct** — *Unfreezing the Robot: Navigation in Dense, Interacting Crowds*.
- Volume/pages: **wrong/incomplete: should include** pp. 797–803; no volume applies.

**CONTEXT**

- `main.tex:48` — **SUPPORTS.** The paper explicitly defines the freezing-robot problem as the planner concluding that forward paths are unsafe and stopping or making unnecessary maneuvers. It does not establish the manuscript’s broader claim that passing-side indecision is the root cause; that is properly presented as this paper’s thesis.

**VERDICT — FIX.**

---

### `mavrogiannis2022momentum`

**EXISTENCE — YES.** [ACM publisher record](https://doi.org/10.1145/3495244) · [DBLP record](https://dblp.org/rec/journals/thri/MavrogiannisATK22.html) · [Author-hosted manuscript](https://chrismavrogiannis.com/pdfs/mavrogiannis2022socialmomentum.pdf)

**METADATA**

- Authors: **wrong: should be** C. Mavrogiannis, P. Alves-Oliveira, **W. Thomason**, and R. A. Knepper. `J. Thomason` is incorrect.
- Year: **correct** — 2022.
- Venue: **correct** — *ACM Transactions on Human-Robot Interaction*.
- Title: **correct**.
- Volume/pages: **correct but expandable** — 11(2), Article 14; the complete extent is 14:1–14:37.

**CONTEXT**

- `main.tex:50` — **WEAK.** The paper supports reinforcing an already established passing side, but it is not empirical evidence for the general human-behavior claim that people maintain every once-chosen passing direction.
- `main.tex:66` — **WEAK.** Social Momentum preserves the sign and increases the magnitude of angular momentum computed from current kinematics; it is not a persistent evidence accumulator with an internal commitment state.
- `main.tex:78` — **SUPPORTS.** The method is a frequent-replanning policy with an angular-momentum objective and sign-preservation rule, so describing it as a partial heuristic mechanism without an explicit commitment lifecycle is fair.

**VERDICT — FIX.** Keep the source, but change “accumulates and maintains” to something like “reinforces and avoids inverting the currently established pairwise passing side.” A separate behavioral source is needed if the human-persistence claim is retained as an empirical assertion.

---

### `mavrogiannis2023survey`

**EXISTENCE — YES.** [DBLP published record](https://dblp.org/rec/journals/thri/MavrogiannisBWZ23) · [Published-paper PDF](https://fluentrobotics.com/pdfs/mavrogiannis2023corechallenges.pdf) · [ACM DOI](https://doi.org/10.1145/3583741)

**METADATA**

- Authors: **correct** — Christoforos Mavrogiannis, Francesca Baldini, Allan Wang, Dapeng Zhao, Pete Trautman, Aaron Steinfeld, Jean Oh.
- Year: **correct** — 2023. This is the published journal version, not merely the 2021 arXiv preprint.
- Venue: **correct** — *ACM Transactions on Human-Robot Interaction*.
- Title: **correct**.
- Volume/pages: **correct** — 12(3), Article 36; equivalently 36:1–36:39.

**CONTEXT**

- `main.tex:66` — **SUPPORTS.** The survey catalogs social-navigation evaluation metrics and explicitly calls for convergence toward a benchmarking protocol. It also warns that no consensus or standardized benchmark yet exists, so “undergoing standardization” must mean an ongoing effort—not an achieved standard.

**VERDICT — KEEP.**

```json
{"per_entry":[{"key":"kirby2010thesis","verdict":"FIX","axis_failures":["METADATA"],"note":"The work is real and supports both uses, but the parenthetical descriptor is not part of the title.","fixed_entry":"\\bibitem{kirby2010thesis} R. Kirby. \\emph{Social Robot Navigation}. PhD thesis, Carnegie Mellon University, Technical Report CMU-RI-TR-10-13, 2010."},{"key":"bhattacharya2012topological","verdict":"FIX","axis_failures":["METADATA"],"note":"Remove the non-title parenthetical; the paper supports the topology claim, with H-signature technically a homology invariant.","fixed_entry":"\\bibitem{bhattacharya2012topological} S. Bhattacharya, M. Likhachev, and V. Kumar. Topological Constraints in Search-based Robot Path Planning. \\emph{Autonomous Robots}, 33(3), pp. 273--290, 2012."},{"key":"dragan2013legibility","verdict":"FIX","axis_failures":["METADATA"],"note":"Kenton Lee's T. initial is missing; both observer-inference uses are appropriate.","fixed_entry":"\\bibitem{dragan2013legibility} A. D. Dragan, K. C. T. Lee, and S. S. Srinivasa. Legibility and Predictability of Robot Motion. In \\emph{Proc. 8th ACM/IEEE Int. Conf. on Human-Robot Interaction (HRI)}, pp. 301--308, 2013."},{"key":"trautman2010","verdict":"FIX","axis_failures":["METADATA"],"note":"Add pages 797--803; the source supports the freezing-robot terminology but not the manuscript's proposed structural cause.","fixed_entry":"\\bibitem{trautman2010} P. Trautman and A. Krause. Unfreezing the Robot: Navigation in Dense, Interacting Crowds. In \\emph{Proc. IEEE/RSJ Int. Conf. on Intelligent Robots and Systems (IROS)}, pp. 797--803, 2010."},{"key":"mavrogiannis2022momentum","verdict":"FIX","axis_failures":["METADATA","CONTEXT"],"note":"J. Thomason should be W. Thomason; retain the source but avoid portraying its angular-momentum objective as a persistent evidence accumulator.","fixed_entry":"\\bibitem{mavrogiannis2022momentum} C. Mavrogiannis, P. Alves-Oliveira, W. Thomason, and R. A. Knepper. Social Momentum: Design and Evaluation of a Framework for Socially Competent Robot Navigation. \\emph{ACM Transactions on Human-Robot Interaction (THRI)}, 11(2), Article 14, pp. 14:1--14:37, 2022."},{"key":"mavrogiannis2023survey","verdict":"KEEP","axis_failures":[],"note":"Published metadata is correct and the survey supports ongoing work toward metric and benchmark standardization, while emphasizing that consensus is not yet achieved.","fixed_entry":null}]}
```
