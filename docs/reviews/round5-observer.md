# Round 5 observer correction

Baseline: 3eaf1a0b08f9b9068efe80dd1e4b03f1014e42cd (PR #1).

Only the observer calculation changes; decision policy, scenario parameters,
seeds 0–49, final-side target, and horizon hold convention are unchanged.
R1 was regenerated on AMD EPYC 9V74 with GCC 13.3, C++17 -O2.
Historical R3 latency measurements remain from the original Ryzen host.

Validation:
- 30 R + 170 L: 3.05 s, uncensored; 60 R + 140 L: 6.05 s.
- 100 R + 100 L remains censored; R/L mirror and empty-input tests pass.
- 1,500 rows retain identical switches, sign-change rate and decision entropy.
- All six aggregate rows match raw data in R1, Korean and English manuscripts.
- Rho sweep re-executed; output is identical to the committed baseline.

Full/intermittent switches at 1.55–2.10 s, not at the end of the horizon.
Its corrected mean t-legible is 4.112 s with 0/50 censored, versus the faulty
10 s / 50/50 report. Full aggregate is 1.326 s / 0 of 250 censored.

The oscillation figure uses only switch counts, which are unchanged; its
existing numeric content remains valid. No physical legibility, safety or
freezing performance is validated by this metric repair.

Build commands (repository root):

```sh
g++ -std=c++17 -O2 -I src/compass_core/include -I src/compass_eval/include src/compass_eval/src/main.cpp src/compass_core/src/*.cpp -o /tmp/compass_eval
g++ -std=c++17 -O2 -I src/compass_core/include -I src/compass_eval/include src/compass_eval/test/test_observer.cpp -o /tmp/test_observer
/tmp/test_observer
/tmp/compass_eval ablation src/compass_eval/results > src/compass_eval/results/R1_ablation.md
python3 scripts/check_paper_numbers.py
python3 scripts/check_observer_results.py
```

ROS/ament integration tests require a ROS 2 Jazzy environment and were not run
in this standalone C++ verification.

PDF: rebuilt as 39 pages using pdfLaTeX (TeX Live 2023), with
`\PassOptionsToPackage{expansion=false}{microtype}` because this environment's
text companion fonts do not support expansion. Missing algorithms/algorithmicx
packages were obtained from CTAN into a temporary TEXINPUTS directory. Two
passes resolved references. The corrected aggregate table was rendered and
visually checked. This is not a new physical evaluation or independent re-review.
