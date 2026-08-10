## `riosmartinez2015survey`

**EXISTENCE — YES.** Verified by [DBLP](https://dblp.org/rec/journals/ijsr/Rios-MartinezSL15) and DOI [10.1007/s12369-014-0251-1](https://doi.org/10.1007/s12369-014-0251-1).

**METADATA**

- Authors: **correct** — Jorge Ríos-Martínez, Anne Spalanzani, Christian Laugier.
- Year: **correct** — 2015. The DOI was registered/online in 2014, but the journal issue is 2015.
- Venue: **correct** — *International Journal of Social Robotics*.
- Title: **correct**.
- Volume/pages: **correct** — 7(2), 137–153. The entry omits issue 2, but the stated volume and pages are correct.

**CONTEXT**

- `main.tex:74` — **SUPPORTS.** The survey treats spaces associated with interacting groups separately and specifically reviews recognizing F-formations and incorporating them into robot navigation.
- `main.tex:139`, forward-extended personal space — **SUPPORTS.** It reviews an egg-shaped personal space that is larger in front and says frontal invasions are more uncomfortable; see the [full-text discussion](https://www.researchgate.net/publication/276881232_From_Proxemics_Theory_to_Socially-Aware_Navigation_A_Survey).
- `main.tex:139`, rear blind spot causing surprise/discomfort — **WRONG.** The searchable full text contains no rear/blind-spot surprise discussion; instead, it emphasizes frontal-space sensitivity and reports mixed findings about frontal versus indirect approaches.
- `main.tex:407` — **SUPPORTS.** The survey defines o-space and p-space, explains that outsiders tend to respect o-space, and connects F-formation recognition to navigation decisions.

**VERDICT — REPLACE.** Keep this entry for the three supported uses, but replace the rear/blind-spot citation with a source that actually studies rear approach, startle, or surprise.

---

## `trautman2015dense`

**EXISTENCE — YES.** Verified by [DBLP](https://dblp.org/rec/journals/ijrr/TrautmanMMK15), the [Caltech repository](https://authors.library.caltech.edu/records/64682-sx438), and DOI [10.1177/0278364914557874](https://doi.org/10.1177/0278364914557874).

**METADATA**

- Authors: **correct** — Pete Trautman, Jeremy Ma, Richard M. Murray, Andreas Krause.
- Year: **correct** — 2015.
- Venue: **correct** — *The International Journal of Robotics Research*.
- Title: **correct**.
- Volume/pages: **correct** — 34(3), 335–356.

**CONTEXT**

- `main.tex:66` — **SUPPORTS.** The paper models cooperative collision avoidance through interacting Gaussian processes and adds multiple goals to represent goal-driven human decision-making, directly supporting the characterization as intent-aware cooperative crowd navigation.

**VERDICT — KEEP.**

---

## `setti2015gcff`

**EXISTENCE — YES.** Verified by the official [PLOS ONE article](https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0123783), DOI [10.1371/journal.pone.0123783](https://doi.org/10.1371/journal.pone.0123783), and [arXiv:1409.2702](https://arxiv.org/abs/1409.2702).

**METADATA**

- Authors: **correct** — Francesco Setti, Chris Russell, Chiara Bassetti, Marco Cristani.
- Year: **correct** — 2015.
- Venue: **correct** — *PLOS ONE*.
- Title: **wrong: should be** “F-Formation Detection: Individuating Free-Standing Conversational Groups in Images.” “(GCFF)” is the method acronym, not part of the title.
- Volume/pages: **correct** — 10(5), article e0123783.

**CONTEXT**

- `main.tex:407` — **WEAK.** The paper unquestionably introduces graph-cut-based GCFF and outputs group assignments and estimated o-space centers, but it does not document an explicit o-space-radius output; that radius must be derived or supported by another detector/source.

**VERDICT — FIX.** Correct the title and avoid implying that GCFF directly supplies a radius unless the implementation derives one.

---

## `moussaid2009experimental`

**EXISTENCE — YES.** Verified by [PubMed](https://pubmed.ncbi.nlm.nih.gov/19439442/), [PMC full text](https://pmc.ncbi.nlm.nih.gov/articles/PMC2839952/), and [arXiv:0908.3131](https://arxiv.org/abs/0908.3131).

**METADATA**

- Authors: **correct** — Mehdi Moussaïd, Dirk Helbing, Simon Garnier, Anders Johansson, Maud Combe, Guy Theraulaz.
- Year: **correct** — 2009.
- Venue: **wrong/incomplete: should be** *Proceedings of the Royal Society B: Biological Sciences*.
- Title: **wrong: should omit** the entire parenthetical beginning “single-population…”. It is editorial commentary, not part of the title.
- Volume/pages: **correct**, but incomplete — 276(1668), 2755–2762.
- Parenthetical factual detail: **wrong/misleading** — the laboratory experiment had 40 subjects, but the Bordeaux field observation tracked 2,670 pedestrians; `N=40` does not describe the combined laboratory/field study.

**CONTEXT**

- `main.tex:139` — **SUPPORTS.** The study finds an avoidance-side preference amplified by mutual interaction and relates the resulting local rules to asymmetric bidirectional lanes; all laboratory and field observations were in Bordeaux, so the warning that it is not cross-cultural evidence is appropriate.

**VERDICT — FIX.**

---

## `mavrogiannis2023winding`

**EXISTENCE — YES.** Verified by [DBLP](https://dblp.org/rec/journals/ral/MavrogiannisBPG23), [arXiv:2109.05084](https://arxiv.org/abs/2109.05084), and DOI [10.1109/LRA.2022.3223024](https://doi.org/10.1109/LRA.2022.3223024).

**METADATA**

- Authors: **correct**.
- Year: **correct** — 2023 for the paginated journal issue; the article was accepted/published online in 2022.
- Venue: **correct** — *IEEE Robotics and Automation Letters*.
- Title: **correct**.
- Volume/pages: **correct** — 8(1), 121–128.
- arXiv statement: **correct** — arXiv:2109.05084, initially submitted in 2021.

**CONTEXT**

- `main.tex:70` — **SUPPORTS.** The paper formalizes passing using pairwise winding/topological invariance and explicitly defines a cost that penalizes switching between different sides of a person.
- `main.tex:78` — **SUPPORTS.** Describing it as a soft topological-invariance mechanism is accurate; it is an MPC cost functional rather than a discrete commitment lifecycle or switching-rate guarantee.

**VERDICT — KEEP.**

---

## `degroot2024topology`

**EXISTENCE — YES.** Verified by the [final published PDF](https://repository.tudelft.nl/file/File_16d27b91-7ae1-437d-aae2-27ee871401b3?preview=1), [DBLP](https://dblp.org/rec/journals/trob/GrootFGA25), [arXiv:2401.06021](https://arxiv.org/abs/2401.06021), and DOI [10.1109/TRO.2024.3475047](https://doi.org/10.1109/TRO.2024.3475047).

**METADATA**

- Authors: **wrong/incomplete: should be** O. de Groot, L. Ferranti, **D. M. Gavrila**, J. Alonso-Mora.
- Year: **wrong: should be 2025** when citing the final paginated volume 41 record. It appeared online/was accepted in 2024, but the final PDF header and DBLP place volume 41, pages 110–126, in 2025.
- Venue: **correct** — *IEEE Transactions on Robotics*.
- Title: **correct**.
- Volume/pages: **correct** — 41, 110–126.
- arXiv statement: **correct** — arXiv:2401.06021 is the 2024 preprint.

**CONTEXT**

- `main.tex:70` — **SUPPORTS.** The method generates distinct homotopy-class guidance trajectories, runs constrained local optimizers in parallel, propagates class identifiers across replanning iterations, and selects an executable trajectory in receding-horizon operation.
- `main.tex:78` — **WEAK.** Parallel per-class optimization and inter-cycle consistency are supported, but “does not formalize commitment itself” overstates the distinction: the paper explicitly defines a consistency parameter \(c_i\), and \(c_i=0\) enforces the previous homotopy class when it remains available. A narrower claim—no track-ID lifecycle or switching-rate bound—would be defensible.

**VERDICT — FIX.** Correct the year and author initials, and narrow the comparative novelty claim.

```json
{"per_entry":[{"key":"riosmartinez2015survey","verdict":"REPLACE","axis_failures":["CONTEXT"],"note":"Valid for proxemics and F-formations, but it does not support the rear/blind-spot surprise claim.","fixed_entry":null},{"key":"trautman2015dense","verdict":"KEEP","axis_failures":[],"note":"Metadata is correct and the intent-aware cooperative-navigation use is supported.","fixed_entry":null},{"key":"setti2015gcff","verdict":"FIX","axis_failures":["METADATA","CONTEXT"],"note":"Remove GCFF from the title; the paper estimates memberships and o-space centers but not an explicit radius output.","fixed_entry":"\\bibitem{setti2015gcff} F. Setti, C. Russell, C. Bassetti, M. Cristani. F-Formation Detection: Individuating Free-Standing Conversational Groups in Images. \\emph{PLOS ONE}, 10(5), e0123783, 2015."},{"key":"moussaid2009experimental","verdict":"FIX","axis_failures":["METADATA"],"note":"The explanatory parenthetical is not part of the title and misleadingly applies N=40 to a study whose field component tracked 2,670 pedestrians.","fixed_entry":"\\bibitem{moussaid2009experimental} M. Moussa\\\"id, D. Helbing, S. Garnier, A. Johansson, M. Combe, G. Theraulaz. Experimental study of the behavioural mechanisms underlying self-organization in human crowds. \\emph{Proceedings of the Royal Society B: Biological Sciences}, 276(1668), pp. 2755--2762, 2009."},{"key":"mavrogiannis2023winding","verdict":"KEEP","axis_failures":[],"note":"Journal and arXiv metadata are correct, and both topological-consistency uses are supported.","fixed_entry":null},{"key":"degroot2024topology","verdict":"FIX","axis_failures":["METADATA","CONTEXT"],"note":"Use 2025 for the final volume-41 citation and qualify the novelty claim because the work explicitly formalizes inter-cycle homotopy consistency.","fixed_entry":"\\bibitem{degroot2024topology} O. de Groot, L. Ferranti, D. M. Gavrila, J. Alonso-Mora. Topology-Driven Parallel Trajectory Optimization in Dynamic Environments. \\emph{IEEE Transactions on Robotics (T-RO)}, 41, pp. 110--126, 2025. Also \\emph{arXiv preprint arXiv:2401.06021}."}]}
```
