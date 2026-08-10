Audit date: 2026-08-10. All six works exist; the main issues are one outdated preprint citation, incomplete Springer/book metadata, and an overstatement that Social-LSTM is multimodal.

### 1. `gao2022evaluation`

**EXISTENCE — YES.** Verified on the [Frontiers article page](https://www.frontiersin.org/journals/robotics-and-ai/articles/10.3389/frobt.2021.721317/full).

**METADATA**

- Authors — correct: Yuxiang Gao and Chien-Ming Huang.
- Year — correct: 2022. Although the volume banner says “Volume 8 — 2021” and the DOI contains 2021, Frontiers dates publication to 12 January 2022 and prescribes the 2022 citation.
- Venue — correct: *Frontiers in Robotics and AI*.
- Title — correct: “Evaluation of Socially-Aware Robot Navigation.”
- Volume — correct: 8.
- Pages/article number — correct: Article 721317; there is no conventional page range.

**CONTEXT**

- `[main.tex:66]` — **SUPPORTS.** The article reviews evaluation metrics for naturalness, discomfort/comfort, and sociability, and explicitly states that no standard evaluation protocols or metrics yet exist, directly supporting the standardization claim. It does not specifically establish decision oscillation as a standard metric, but the citation attaches primarily to the broader evaluation-standardization sentence.

**VERDICT — KEEP.**

### 2. `francis2023principles`

**EXISTENCE — YES.** The supplied [2023 arXiv record](https://arxiv.org/abs/2306.16740) exists, but the work was subsequently published in [ACM THRI, DOI 10.1145/3700599](https://doi.org/10.1145/3700599).

**METADATA**

- Authors — correct as abbreviated: Anthony Francis, Claudia Pérez-D’Arpino, Chengshu Li, Fei Xia, Alexandre Alahi, Rachid Alami, et al.
- Year — wrong: should be **2025** for the archival journal publication.
- Venue — wrong/outdated: should be *ACM Transactions on Human-Robot Interaction*, not an arXiv preprint.
- Title — correct.
- Volume — wrong/omitted: should be **14(2)**.
- Pages/article number — wrong/omitted: should be **Article 34, 65 pages** (also represented as 34:1–34:65).

**CONTEXT**

- `[main.tex:66]` — **SUPPORTS.** The article explicitly proposes common evaluation principles—including comfort and legibility—along with guidelines and a metrics framework intended to make social-navigation evaluations comparable.

**VERDICT — FIX.**

Corrected entry:

```latex
\bibitem{francis2023principles}
A. Francis, C. P\'erez-D'Arpino, C. Li, F. Xia, A. Alahi, R. Alami, et al. Principles and Guidelines for Evaluating Social Robot Navigation Algorithms. \emph{ACM Transactions on Human-Robot Interaction}, 14(2), Article 34, 65 pages, 2025.
```

### 3. `alahi2016sociallstm`

**EXISTENCE — YES.** Verified through the [CVF CVPR record](https://openaccess.thecvf.com/content_cvpr_2016/html/Alahi_Social_LSTM_Human_CVPR_2016_paper.html) and [DBLP](https://dblp.org/rec/conf/cvpr/AlahiGRRLS16).

**METADATA**

- Authors — correct.
- Year — correct: 2016.
- Venue — correct: CVPR 2016.
- Title — correct.
- Volume — correct/not applicable; CVPR proceedings do not use a journal-style volume here.
- Pages — correct: 961–971.

**CONTEXT**

- `[main.tex:68]` — **WEAK.** Social-LSTM predicts a bivariate Gaussian distribution at each future step, so “probabilistic” or “predicts a distribution” is supported; however, that is a unimodal Gaussian model, so grouping it under “multimodal predictors” is inaccurate. This conclusion follows from the [paper’s single-bivariate-Gaussian output formulation](https://www.cv-foundation.org/openaccess/content_cvpr_2016/papers/Alahi_Social_LSTM_Human_CVPR_2016_paper.pdf).

**VERDICT — REPLACE.** If “multimodal” is retained, replace Social-LSTM with an actually multimodal predictor. Alternatively, change the prose to “learning-based probabilistic predictors,” in which case this citation becomes appropriate.

### 4. `gupta2018socialgan`

**EXISTENCE — YES.** Verified through the [CVF CVPR record](https://openaccess.thecvf.com/content_cvpr_2018/html/Gupta_Social_GAN_Socially_CVPR_2018_paper.html) and [DBLP](https://dblp.org/rec/conf/cvpr/GuptaJFSA18).

**METADATA**

- Authors — correct.
- Year — correct: 2018.
- Venue — correct: CVPR 2018.
- Title — correct.
- Volume — correct/not applicable.
- Pages — correct: 2255–2264.

**CONTEXT**

- `[main.tex:68]` — **SUPPORTS.** The work explicitly motivates trajectory prediction as multimodal and uses a GAN plus variety loss to generate diverse, socially plausible future trajectories.

**VERDICT — KEEP.**

### 5. `salzmann2020trajectron`

**EXISTENCE — YES.** Verified on the [Springer ECCV publication page](https://link.springer.com/chapter/10.1007/978-3-030-58523-5_40), [DBLP](https://dblp.org/rec/conf/eccv/SalzmannICP20), and [arXiv](https://arxiv.org/abs/2001.03093).

**METADATA**

- Authors — correct.
- Year — correct: 2020.
- Venue — correct but incomplete: *Computer Vision – ECCV 2020*, Part XVIII, published by Springer.
- Title — correct.
- Volume — wrong/omitted under the requested metadata fields: should be *Lecture Notes in Computer Science*, vol. **12363**.
- Pages — correct: 683–700.

**CONTEXT**

- `[main.tex:68]` — **SUPPORTS.** Trajectron++ explicitly produces multimodal probabilistic trajectory forecasts and was designed for integration with downstream planning, decision-making, and control modules.

**VERDICT — FIX.**

Corrected entry:

```latex
\bibitem{salzmann2020trajectron}
T. Salzmann, B. Ivanovic, P. Chakravarty, M. Pavone. Trajectron++: Dynamically-Feasible Trajectory Forecasting with Heterogeneous Data. In \emph{Computer Vision--ECCV 2020, Part XVIII}, Lecture Notes in Computer Science, vol. 12363, pp. 683--700, Springer, 2020.
```

### 6. `kendon1990conducting`

**EXISTENCE — YES.** Verified through [WorldCat](https://search.worldcat.org/fr/title/Conducting-interaction-%3A-patterns-of-behavior-in-focused-encounters/oclc/877783168) and [Google Books](https://books.google.com/books/about/Conducting_Interaction.html?id=7-8zAAAAIAAJ).

**METADATA**

- Author — correct: Adam Kendon.
- Year — correct: 1990.
- Venue/publisher — correct: Cambridge University Press.
- Title — wrong: “(F-formation)” is not part of the book title and should be removed.
- Volume — wrong/omitted under the requested fields: the book is volume 7 of *Studies in Interactional Sociolinguistics*.
- Pages — no cited page range is required for the whole monograph; total length is 292 pages.

**CONTEXT**

- `[main.tex:74]` — **WEAK.** Kendon supports the formalization of F-formations and o-/p-space, but does not itself formulate the robotics rule “groups that must not be split”; that normative navigation interpretation needs the robotics source already cited afterward or another explicit group-navigation source.
- `[main.tex:407]` — **WEAK.** Kendon supports the o-space concept, but o-space is the central shared transactional space—not the group’s outer boundary—and a center/radius representation is an engineering approximation rather than Kendon’s definition. Accessible page-cited discussion of the definitions appears in this [University of Cambridge report](https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-771.pdf).

Recommended prose would distinguish “estimated o-space geometry” from “group boundary” and attribute the no-splitting rule to robotics literature.

**VERDICT — FIX.** Retain Kendon as the foundational source, correct the bibliographic title, and narrow the contextual wording.

Corrected entry:

```latex
\bibitem{kendon1990conducting}
A. Kendon. \emph{Conducting Interaction: Patterns of Behavior in Focused Encounters}. Studies in Interactional Sociolinguistics, vol. 7. Cambridge University Press, 1990.
```

```json
{"per_entry":[{"key":"gao2022evaluation","verdict":"KEEP","axis_failures":[],"note":"Frontiers metadata is correct and the review directly supports the evaluation-metrics standardization claim.","fixed_entry":null},{"key":"francis2023principles","verdict":"FIX","axis_failures":["METADATA"],"note":"The 2023 preprint exists, but the archival citation is ACM THRI 14(2), Article 34, 2025.","fixed_entry":"\\bibitem{francis2023principles} A. Francis, C. P\\'erez-D'Arpino, C. Li, F. Xia, A. Alahi, R. Alami, et al. Principles and Guidelines for Evaluating Social Robot Navigation Algorithms. \\emph{ACM Transactions on Human-Robot Interaction}, 14(2), Article 34, 65 pages, 2025."},{"key":"alahi2016sociallstm","verdict":"REPLACE","axis_failures":["CONTEXT"],"note":"Metadata is correct, but Social-LSTM uses a unimodal bivariate Gaussian and does not support calling the listed methods collectively multimodal.","fixed_entry":null},{"key":"gupta2018socialgan","verdict":"KEEP","axis_failures":[],"note":"CVPR metadata is correct and Social-GAN directly supports multimodal, socially plausible trajectory prediction.","fixed_entry":null},{"key":"salzmann2020trajectron","verdict":"FIX","axis_failures":["METADATA"],"note":"Add the archival Springer proceedings information: ECCV Part XVIII, LNCS volume 12363.","fixed_entry":"\\bibitem{salzmann2020trajectron} T. Salzmann, B. Ivanovic, P. Chakravarty, M. Pavone. Trajectron++: Dynamically-Feasible Trajectory Forecasting with Heterogeneous Data. In \\emph{Computer Vision--ECCV 2020, Part XVIII}, Lecture Notes in Computer Science, vol. 12363, pp. 683--700, Springer, 2020."},{"key":"kendon1990conducting","verdict":"FIX","axis_failures":["METADATA","CONTEXT"],"note":"Remove the non-title '(F-formation)' suffix and avoid equating o-space with an outer group boundary or attributing the robotic no-split rule directly to Kendon.","fixed_entry":"\\bibitem{kendon1990conducting} A. Kendon. \\emph{Conducting Interaction: Patterns of Behavior in Focused Encounters}. Studies in Interactional Sociolinguistics, vol. 7. Cambridge University Press, 1990."}]}
```
