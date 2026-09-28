# Observer result versions (issue #3 audit record)

This directory keeps every archived version of the offline decision-stream
observer output so the numerical correction and the later metric-definition
change can be audited separately. All three versions score the **same 1,500
decision streams** (6 variants x 5 scenarios x 50 seeds). The decision policy,
scenario scripts, seeds, prior 0.5, misread likelihood eps = 0.2, threshold
p* = 0.9 and final-output-side target are identical; switch count, sign-change
rate and decision entropy are byte-identical across the three raw CSVs.

| Version | Rule | Raw CSV | Role |
|---|---|---|---|
| `9fe495a` | probability-space recursion; posterior rounds to exactly 1.0 and absorbs later reversals | [`ablation_raw_9fe495a_probability.csv`](ablation_raw_9fe495a_probability.csv) (`git show 9fe495a:src/compass_eval/results/ablation_raw.csv`) | original, numerically defective |
| `5343d45` (PR #12) | log-odds recursion, same endpoint-suffix rule; a crossing at the final sample counts as attained | [`ablation_raw_5343d45_logodds_only.csv`](ablation_raw_5343d45_logodds_only.csv) (`git show 5343d45:src/compass_eval/results/ablation_raw.csv`) | saturation-only numerical correction |
| current | log-odds, plus at least 0.30 s of observed follow-up after the first sample of the final confident suffix | [`../ablation_raw.csv`](../ablation_raw.csv) | metric-definition change, reported in R1 and the manuscript |

**Name and status of the reported statistic.** The reported value is an
*endpoint-suffix time* `t_sfx`: the first sample of the maximal final run of
samples whose posterior for the final output side is at least p*. It is not the
manuscript's `t_legible` with a fixed hold window `Delta_hold`, it is not a
"strictest" or conservative hold condition, and it is not a human or motion
legibility measure. The 0.30 s follow-up value was chosen **after** the PR #12
review found 22 final-sample-only uncensored rows; it was not preregistered and
no sensitivity analysis over the follow-up length is reported. The CSV column
name `t_legible_s` is retained only for archive compatibility.

**Per-row comparison.** [`observer_versions.csv`](observer_versions.csv) has one
row per stream with `t_sfx` and censoring under each version. It is regenerated
from source by

```sh
g++ -std=c++17 -O2 -I src/compass_core/include -I src/compass_eval/include \
  src/compass_eval/src/main.cpp src/compass_core/src/*.cpp -o /tmp/compass_eval
/tmp/compass_eval observer_versions /tmp/ov   # writes /tmp/ov/observer_versions.csv
python3 scripts/check_observer_versions.py    # archives == regenerated columns
```

`scripts/test_compass_observability.sh` regenerates the file and requires byte
equality; `scripts/check_observer_versions.py` requires every version column to
equal its archived raw CSV. SHA-256 values of all result files are recorded in
[`../PROVENANCE.md`](../PROVENANCE.md).

Summary (mean includes censored rows at the 10 s horizon value; not an estimate
of eventual attainment):

<!-- observer-summary:begin -->
| Variant | 9fe495a probability space: mean t_sfx (s) / censored | 5343d45 log-odds only: mean t_sfx (s) / censored | current log-odds + 0.30 s follow-up: mean t_sfx (s) / censored |
|---|---|---|---|
| full (제안) | 2.50 / 50/250 | 1.33 / 0/250 | 1.33 / 0/250 |
| -hysteresis | 2.39 / 49/250 | 1.00 / 0/250 | 1.00 / 0/250 |
| -progress hardening | 2.50 / 50/250 | 1.31 / 0/250 | 1.31 / 0/250 |
| -accumulator (즉시 argmin) | 3.61 / 74/250 | 3.59 / 48/250 | 3.60 / 72/250 |
| simple-dwell (O4) | 2.26 / 50/250 | 2.26 / 50/250 | 2.26 / 50/250 |
| -class correspondence | 3.62 / 73/250 | 3.59 / 48/250 | 3.60 / 71/250 |

- prob_9fe495a -> logodds_5343d45: 200 rows change observer output (200 uncensored, 0 newly censored).
- logodds_5343d45 -> followup030_current: 47 rows change observer output (0 uncensored, 47 newly censored).
<!-- observer-summary:end -->

The 47 rows changed by the follow-up rule all belong to the immediate-argmin and
synthetic correspondence-loss comparators (22 at 9.95 s, 25 at 9.75 or 9.85 s
in the log-odds-only version). The proposed method and the two knob-only
ablations have identical values under the log-odds-only and current versions.
