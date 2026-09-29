# Result provenance (issue #11)

SHA-256 of every committed result file in this directory, with the command and
source revision that produced it. `scripts/check_repo_consistency.py` recomputes
each hash and requires that every result file is listed here, so a changed or
new result file fails the gate until this record is updated.

<!-- provenance-hashes:begin -->
| File | SHA-256 | Content | Produced by |
|---|---|---|---|
| `R1_ablation.md` | `dde5026f68438731a33b15f8a901d1b9165e8a60cdd5811def3f6540498abc7b` | R1 tables (stdout of `compass_eval ablation`) | numbers from `7f5ce82` run; headers renamed to `t_sfx` in #3 (numbers unchanged) |
| `R3_latency.md` | `884f678c01c5bf83588eec4278b04c93b7afd0f3517cd0a75aabef459385de5c` | R3 archived decision-core latency | `compass_eval latency`, source `9fe495a`, AMD Ryzen 5 5500GT; machine dependent, not reproducible byte-for-byte |
| `R5_rho_sweep.md` | `b1c5e558441ca8472a33569006bb6541e20d6f2e228e019e6f87bb1610dbcafd` | R5 scripted v_lat sweep (clean_commit) | `compass_eval rho_sweep`; stdout reproduced byte-identically on this branch |
| `ablation_raw.csv` | `a6c621e71979e16e31fc9e8067808ac9406a13e595481f7b4e5d1be7bb97848b` | R1 raw, current observer (log-odds + post-hoc 0.30 s follow-up) | `compass_eval ablation <dir>`; source `7f5ce82`, rerun on AMD EPYC 9V74, GCC 13.3 |
| `closed_loop_diagnostic/summary.json` | `743fd301c17bac873b1874c3a33a46574b4f8e1c4a2d6b19cc24ff74100ca0a8` | scripted-cost unicycle diagnostic summary (contains trace SHA-256) | `scripts/test_completion_gates.sh`; source `0921b70` |
| `closed_loop_diagnostic/validation.txt` | `6d90cbc9b8d449fd8dcc2f4eb2b91d07a6b10bddd444016f718d72cfbbfc6885` | historical validation transcript | as above |
| `observer_versions/ablation_raw_5343d45_logodds_only.csv` | `bb86278465b46581db2a74f1f4cec10e75b71dc26e921d3db1f6be7cdebf4237` | R1 raw, saturation-only log-odds observer (PR #12) | `git show 5343d45:src/compass_eval/results/ablation_raw.csv` |
| `observer_versions/ablation_raw_9fe495a_probability.csv` | `2a37f94cfbd3eb7491dac5c0b0fa7923d668125ec5bd1d5af29e2a34009442d2` | R1 raw, original probability-space observer | `git show 9fe495a:src/compass_eval/results/ablation_raw.csv` |
| `observer_versions/observer_versions.csv` | `94fee5e9ff7820046e80d487d341950fe7aaf4fe4ac8dd98f05243ab3582ff7d` | per-row comparison of the three observer versions | `compass_eval observer_versions <dir>` |
| `responsive_profile/paired_results.json` | `e88b597d02380bdc2b471359ce81f2ae03ab0a5004ccef4e072af627239916c6` | 3,750-run paired scripted sweep, per-seed | `scripts/run_response_comparison.py` via `scripts/test_compass_response.sh`; source `41e7837` |
| `responsive_profile/summary.json` | `c2fb16941cfffd0e1741ab9c5cf1467e0c15173c326a7bbc551c294faa0b321c` | paired sweep summary | as above |
| `responsive_profile/validation.txt` | `e40a3d341ba729dd551e2cb6cfd495f269d3c4fd0858478e7a919c159bc7e319` | historical validation transcript | as above |
| `stdlib_sensitivity/R1_ablation_libcxx.md` | `c487b7c1131e86515e7191a762805d899cc6d03d15761fff8699990dff4b07e6` | R1 tables, current harness built against LLVM libc++ (CPU line removed) | `scripts/check_stdlib_sensitivity.sh --write`; base `6ea9ac3`, clang 23.1.2 / libc++ 230102 |
| `stdlib_sensitivity/R5_rho_sweep_libcxx.md` | `5c80555210ab9c7696ef57df7936c857ba52a75911c838fd2e8c2961dfd2342f` | R5 sweep, LLVM libc++ build | as above |
| `stdlib_sensitivity/ablation_raw_libcxx.csv` | `a702c7feeccee7aff4a93b1d421214245300770cc21555938235e42b4006e2d8` | R1 raw, LLVM libc++ build (sensitivity record, not canonical) | as above |
<!-- provenance-hashes:end -->

## Build, parameters and seeds

- Harness build (no ROS): `g++ -std=c++17 -O2 -I src/compass_core/include
  -I src/compass_eval/include src/compass_eval/src/main.cpp
  src/compass_core/src/*.cpp -o /tmp/compass_eval`. The ROS build uses the
  package CMake defaults of the colcon workspace.
- Offline R1: 5 scenarios x seeds 0-49 x 6 variants, `dt = 0.05` s, 200
  cycles (10 s), scripted progress input `v_lat = 0.05`, `L_plan = 1.0` m
  (forward-speed progress proxy), default `compass::Knobs` (manuscript
  Table `tab:params`). Observer: prior 0.5, `eps = 0.2`, `p* = 0.9`,
  final-output-side target; version rules in
  [`observer_versions/README.md`](observer_versions/README.md).
- R5: `clean_commit`, seeds 0-49, `v_lat` in {0.05, 0.10, 0.20, 0.35, 0.50}.
- Random numbers: `std::mt19937` seeded per trial with `std::normal_distribution`
  (and `std::uniform_int_distribution` for the synthetic comparator); exact values
  are specific to libstdc++. An LLVM libc++ build of the same source changes 425
  of 1,500 rows; see [`stdlib_sensitivity/README.md`](stdlib_sensitivity/README.md)
  and `scripts/check_stdlib_sensitivity.sh`.

## Verification on this branch

On branch `fix/round5-open` (base `d543dc2`, PR #15 head), GCC 9.4.0 (Ubuntu
20.04, x86_64, AMD Ryzen 5 5500GT) with the command above reproduces
`ablation_raw.csv`, `observer_versions/observer_versions.csv` and the R5 stdout
byte-for-byte, and the R1 stdout except for the recorded CPU line and output
path. `scripts/test_compass_observability.sh` performs the CSV comparisons.
R3 is an archived timing measurement and is not expected to reproduce exactly.
