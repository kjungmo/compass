I assessed group citations individually: a source does not receive credit for a claim supported only by its co-cited source.

### `fox1997dwa`

**1. EXISTENCE — YES.** Verified by the [IEEE DOI record](https://doi.org/10.1109/100.580977), [DBLP](https://dblp.org/rec/journals/ram/FoxBT97), and the [authors’ University of Washington record](https://rse-lab.cs.washington.edu/abstracts/colli-ieee.abstract.html).

**2. METADATA**

- Authors: correct — Dieter Fox, Wolfram Burgard, Sebastian Thrun.
- Year: correct — 1997.
- Venue: correct — *IEEE Robotics & Automation Magazine*.
- Title: correct.
- Volume/issue/pages: correct — 4(1), 23–33.

**3. CONTEXT**

- `main.tex:62` — **WEAK.** Fox et al. directly support selecting reachable velocity commands from a dynamic window, but the paper presents DWA as reactive collision avoidance and does not establish the stronger causal claim that it is “memoryless” and therefore vulnerable to oscillation; that limitation needs a later evaluation or separate source.

**VERDICT — REPLACE.** Retain Fox et al. for the description of DWA, but replace or supplement the citation attached to the memorylessness/oscillation claim.

---

### `rosmann2017teb`

**1. EXISTENCE — YES.** Verified by the [Elsevier article record](https://www.sciencedirect.com/science/article/pii/S0921889016300495) and [DBLP](https://dblp.org/rec/journals/ras/RosmannHB17).

**2. METADATA**

- Authors: correct — Christoph Rösmann, Frank Hoffmann, Torsten Bertram.
- Year: correct — 2017 for the volume-of-record publication; the DOI contains 2016 because it was processed online earlier.
- Venue: correct — *Robotics and Autonomous Systems*.
- Title: **wrong: should be “Integrated online trajectory planning and optimization in distinctive topologies.”** “(TEB)” is not part of the title.
- Volume/pages: correct — 88, 142–153.
- DOI: 10.1016/j.robot.2016.11.007.

**3. CONTEXT**

- `main.tex:62` — **WEAK.** The paper clearly maintains and simultaneously optimizes multiple topologically distinctive candidates, but I could not verify in the paper that temporal consistency is specifically obtained “via switching costs”; the associated ROS implementation has a `selection_cost_hysteresis` parameter, but that is narrower than the prose claim.
- `main.tex:78` — **WEAK.** Per-class parallel optimization and per-cycle selection are supported, but labeling that selection “commitment” overstates the paper, which selects the currently best trajectory and does not formalize a commitment state or commitment rule.

**VERDICT — REPLACE.** The work remains appropriate for the narrower parallel-topology claim, but the title and both commitment/switching-cost descriptions need correction or additional sourcing.

---

### `williams2017mppi`

**1. EXISTENCE — YES.** Verified by [IEEE Xplore](https://ieeexplore.ieee.org/document/7989202), [DBLP](https://dblp.org/rec/conf/icra/WilliamsWGDRBT17), and the [author-hosted paper](https://homes.cs.washington.edu/~bboots/files/InformationTheoreticMPC.pdf).

**2. METADATA**

- Authors: correct — Grady Williams, Nolan Wagener, Brian Goldfain, Paul Drews, James M. Rehg, Byron Boots, Evangelos A. Theodorou.
- Year: correct — 2017.
- Venue: correct — 2017 IEEE International Conference on Robotics and Automation (ICRA).
- Title: correct.
- Volume/pages: no volume applies; pages 1714–1721 are correct.
- DOI: 10.1109/ICRA.2017.7989202.

**3. CONTEXT**

- `main.tex:62` — **WEAK.** The work explicitly presents MPPI as sampling-based continuous MPC and contains no social passing-class representation, but it does not claim that ordinary MPPI inherently produces smooth solutions; “smoothness” is an unsupported embellishment here.

**VERDICT — REPLACE.** Keep this source for “sampling-based MPC,” but revise the smoothness claim or cite work that specifically evaluates or enforces smooth MPPI controls.

---

### `helbing1995social`

**1. EXISTENCE — YES.** Verified by the [American Physical Society record](https://journals.aps.org/pre/abstract/10.1103/PhysRevE.51.4282) and [PubMed](https://pubmed.ncbi.nlm.nih.gov/9963139/).

**2. METADATA**

- Authors: correct — Dirk Helbing and Péter Molnár.
- Year: correct — 1995 for the journal article; the 1998 arXiv upload does not change the publication year.
- Venue: correct — *Physical Review E*.
- Title: correct.
- Volume/issue/pages: volume 51 and pages 4282–4286 are correct; the issue is 5.
- DOI: 10.1103/PhysRevE.51.4282.

**3. CONTEXT**

- `main.tex:64` — **WEAK.** The article models pedestrian reactions through desired-velocity, repulsive-distance, and attractive-force terms, but it neither studies robot navigation nor demonstrates mitigation of the “freezing robot” problem.

**VERDICT — REPLACE.** Retain it as the foundational Social Force Model citation, but use a robot social-navigation study for the freezing-mitigation claim.

---

### `vandenberg2011rvo`

**1. EXISTENCE — YES.** Verified by the [official ORCA project/publication page](https://gamma-web.iacs.umd.edu/ORCA/) and [Springer’s proceedings record](https://link.springer.com/book/10.1007/978-3-642-19457-3).

**2. METADATA**

- Authors: correct — Jur van den Berg, Stephen J. Guy, Ming Lin, Dinesh Manocha.
- Year: correct — 2011 for the published Springer chapter; the underlying 14th ISRR meeting occurred in 2009.
- Venue: correct in substance — *Robotics Research: The 14th International Symposium ISRR*, in *Springer Tracts in Advanced Robotics*.
- Title: **wrong: should be “Reciprocal n-Body Collision Avoidance.”** “(RVO/ORCA)” is not part of the title.
- Volume/pages: pages 3–19 are correct; series volume 70 is missing.
- DOI: 10.1007/978-3-642-19457-3_1.

**3. CONTEXT**

- `main.tex:64` — **SUPPORTS.** The work explicitly assigns each agent half the responsibility for pairwise avoidance and reports smooth, collision-free movement through congestion, supporting the reciprocity component and its use against oscillatory/congested behavior.

**VERDICT — FIX.** The use is appropriate, but remove the parenthetical title addition and supply the series metadata.

---

### `hall1966hidden`

**1. EXISTENCE — YES.** Verified by the [WorldCat first-edition record](https://search.worldcat.org/title/The-hidden-dimension/oclc/203769) and the [CCA library catalog](https://www.cca.qc.ca/fr/recherche/details/library/publication/00203769).

**2. METADATA**

- Author: correct — Edward T. Hall.
- Year: correct — 1966.
- Venue/publisher: correct — Doubleday, Garden City, New York.
- Title: **wrong: should be “The Hidden Dimension.”** “(proxemics)” is an editorial description, not part of the title.
- Volume/pages: not applicable; the first edition contains xii + 201 pages.

**3. CONTEXT**

- `main.tex:64` — **WEAK.** Hall identifies qualitative and culture-dependent interpersonal distance zones, but he does not formulate or quantify a robotic cost field; that transformation must be credited to Kirby or another robotics source.
- `main.tex:139` — **WEAK.** Hall supports the existence of intimate, personal, social, and public distance zones, but not a forward-extended asymmetric Gaussian geometry, so he cannot directly ground the claimed shape.

**VERDICT — REPLACE.** Hall is suitable for qualitative proxemic zones and cultural variability, but the cost-field and forward-asymmetry claims require robotics/HRI sources; the title also needs correction if Hall remains elsewhere.

```json
{"per_entry":[{"key":"fox1997dwa","verdict":"REPLACE","axis_failures":["CONTEXT"],"note":"Canonical for DWA command selection, but not direct support for the asserted memorylessness-to-oscillation claim.","fixed_entry":null},{"key":"rosmann2017teb","verdict":"REPLACE","axis_failures":["METADATA","CONTEXT"],"note":"Drop the non-title '(TEB)'; the paper supports parallel topology optimization but not the stronger switching-cost and formal commitment wording.","fixed_entry":null},{"key":"williams2017mppi","verdict":"REPLACE","axis_failures":["CONTEXT"],"note":"Correct MPPI record and sampling citation, but the cited paper does not establish inherent solution smoothness.","fixed_entry":null},{"key":"helbing1995social","verdict":"REPLACE","axis_failures":["CONTEXT"],"note":"Foundational pedestrian Social Force Model, but it does not establish mitigation of robot freezing.","fixed_entry":null},{"key":"vandenberg2011rvo","verdict":"FIX","axis_failures":["METADATA"],"note":"The reciprocal-congestion context is supported, but '(RVO/ORCA)' is not in the title and series volume 70 is missing.","fixed_entry":"\\bibitem{vandenberg2011rvo} J. van den Berg, S. J. Guy, M. Lin, D. Manocha. Reciprocal n-Body Collision Avoidance. In C. Pradalier, R. Siegwart, G. Hirzinger (eds.), \\emph{Robotics Research: The 14th International Symposium ISRR}, Springer Tracts in Advanced Robotics, vol. 70, pp. 3--19, Springer, 2011."},{"key":"hall1966hidden","verdict":"REPLACE","axis_failures":["METADATA","CONTEXT"],"note":"Hall supports qualitative, culture-dependent proxemic zones, not robotic cost fields or a forward-extended Gaussian; '(proxemics)' is not part of the title.","fixed_entry":null}]}
```
