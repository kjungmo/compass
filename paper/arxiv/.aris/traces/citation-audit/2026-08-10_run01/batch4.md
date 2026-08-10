## `kuderer2012feature`

**1. EXISTENCE — YES.** Verified by the [official RSS proceedings record](https://www.roboticsproceedings.org/rss08/p25.html) and [DBLP](https://dblp.org/rec/conf/rss/KudererKSB12).

**2. METADATA**

- Authors: **correct** — Markus Kuderer, Henrik Kretzschmar, Christoph Sprunk, Wolfram Burgard.
- Year: **correct — 2012**. RSS VIII occurred in July 2012; the later 2013 MIT Press book date does not change the conference-paper year.
- Venue: **correct** — *Proceedings of Robotics: Science and Systems*, RSS VIII.
- Title: **correct**.
- Volume/pages: **correct as omitted** — the official RSS BibTeX supplies no conventional volume or page range; “VIII” may optionally be included as the proceedings edition.
- Optional identifier: DOI `10.15607/RSS.2012.VIII.025`.

**3. CONTEXT**

- `main.tex:72` — **SUPPORTS.** The [paper](https://www.roboticsproceedings.org/rss08/p25.pdf) learns feature weights for a maximum-entropy distribution over joint trajectories, represents passing-side choices as topological variants, and uses the model for online robot navigation. It informally describes the robot as “commit[ting]” to a variant and later changing sides, but supplies no lifecycle-based temporal commitment rule or switching-rate bound.

**VERDICT — KEEP.**

---

## `kretzschmar2016irl`

**1. EXISTENCE — YES.** Verified by the [official SAGE/IJRR article page](https://journals.sagepub.com/doi/10.1177/0278364915619772) and [DBLP](https://dblp.org/rec/journals/ijrr/KretzschmarSSB16).

**2. METADATA**

- Authors: **correct** — Henrik Kretzschmar, Markus Spies, Christoph Sprunk, Wolfram Burgard.
- Year: **correct — 2016**. First published online July 11, 2016; issue published September 2016.
- Venue: **correct** — *The International Journal of Robotics Research*.
- Title: **correct**.
- Volume: **correct — 35**.
- Issue: **correct — 11**.
- Pages: **correct — 1289–1307**.
- DOI: `10.1177/0278364915619772`.

**3. CONTEXT**

- `main.tex:72` — **SUPPORTS.** The publisher abstract explicitly describes a learned mixture distribution over discrete left/right decisions and continuous trajectory variance, feature-expectation matching, imitation of pedestrian or teleoperated behavior, and deployment on a real robot; it reports no temporal commitment lifecycle or switching-rate result.

**VERDICT — KEEP.**

---

## `cao2019dynamicchannel`

**1. EXISTENCE — YES.** Verified by the [CMU Robotics Institute publication record](https://www.ri.cmu.edu/publications/dynamic-channel-a-planning-framework-for-crowd-navigation/), [DBLP](https://dblp.org/rec/conf/icra/CaoTI19), [arXiv](https://arxiv.org/abs/1903.00143), and IEEE DOI [`10.1109/ICRA.2019.8794192`](https://doi.org/10.1109/ICRA.2019.8794192).

**2. METADATA**

- Authors: **wrong: should be C. Cao, P. Trautman, S. Iba** — Chao Cao and Soshi Iba are incorrectly given as “Z. Cao” and “K. Iba.”
- Year: **correct — 2019**.
- Venue: **correct** — 2019 IEEE International Conference on Robotics and Automation (ICRA).
- Title: **correct**.
- Volume: **not applicable**.
- Pages: **correct — 5551–5557**.

Corrected entry:

```latex
\bibitem{cao2019dynamicchannel} C. Cao, P. Trautman, S. Iba. Dynamic Channel: A Planning Framework for Crowd Navigation. In \emph{Proc. IEEE Int. Conf. on Robotics and Automation (ICRA)}, pp. 5551--5557, 2019.
```

**3. CONTEXT**

- `main.tex:70` — **SUPPORTS.** The [full paper](https://arxiv.org/pdf/1903.00143) constructs a Delaunay triangulation and dual graph, associates graph paths/channels with homotopy classes, incorporates pedestrian dynamics through timed A*, and outputs a selected channel/path; the paper replans at 50–100 Hz without defining cross-cycle commitment or a switching-rate guarantee. The “one topology per cycle” characterization is a fair inference from the planner output rather than the authors’ terminology.

- `main.tex:78` — **WEAK.** It supports “topological-corridor selection,” but not the stronger assertion that this work recognized temporal passing-side consistency or induced such consistency: its example explicitly replans from passing on the right to passing on the left when the crowd changes.

**VERDICT — REPLACE.** Retain the corrected citation at line 70, but remove or replace its line-78 use unless that sentence is weakened to describe merely a related per-cycle topological mechanism.

```json
{"per_entry":[{"key":"kuderer2012feature","verdict":"KEEP","axis_failures":[],"note":"Verified RSS 2012 maximum-entropy joint-trajectory model; metadata and the narrowly stated no-switching-bound context are sound.","fixed_entry":null},{"key":"kretzschmar2016irl","verdict":"KEEP","axis_failures":[],"note":"Official IJRR metadata is exact and the mixture-distribution/IRL context is directly supported.","fixed_entry":null},{"key":"cao2019dynamicchannel","verdict":"REPLACE","axis_failures":["METADATA","CONTEXT"],"note":"Correct authors to C. Cao and S. Iba; keep at line 70 but replace or remove the weak temporal-consistency use at line 78.","fixed_entry":null}]}
```
