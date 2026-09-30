# Standard-library sensitivity (LLVM libc++ rerun)

The offline harness seeds one `std::mt19937` per trial, but draws the scenario
perturbations with `std::normal_distribution` and the synthetic
correspondence-loss comparator's coin with `std::uniform_int_distribution`.
The C++ standard fixes the engine's output sequence, not these distribution
algorithms. libstdc++ and LLVM libc++ both use the Marsaglia polar method for
the normal and consume the same generator words per accepted pair, but
libstdc++ returns the second member of the pair and libc++ the first; they also
derive the coin from different generator bits. Because the harness constructs a
new `normal_distribution` every cycle, each libc++ perturbation is exactly the
pair member that libstdc++ discards. A persistent distribution would still
differ, because it would emit each pair in the opposite order.

The files here are the **unchanged current harness** built against LLVM libc++.
They are a sensitivity record, not a replacement for the canonical
[`../ablation_raw.csv`](../ablation_raw.csv), `R1_ablation.md` or
`R5_rho_sweep.md`, which remain the libstdc++ results reported in the manuscript.

| File | Content |
|---|---|
| `ablation_raw_libcxx.csv` | R1 raw, same schema and row order as `../ablation_raw.csv` |
| `R1_ablation_libcxx.md` | stdout of `compass_eval ablation`, CPU line removed, raw-CSV pointer renamed to `ablation_raw_libcxx.csv` |
| `R5_rho_sweep_libcxx.md` | stdout of `compass_eval rho_sweep` |

## What changes

425 of the 1,500 rows differ. For the proposed method and the two knob-only
ablations (`-hysteresis`, `-progress hardening`), the switch count, sign-change
rate and decision entropy are identical in all 750 rows; only the observer
statistic `t_sfx` moves (for example the full-method mean 1.33 s becomes 1.34 s).
Switch counts change only for the comparator policies: 120 immediate-argmin rows,
132 synthetic correspondence-loss rows and 1 simple-dwell row.
`scripts/check_stdlib_sensitivity.sh` asserts these row counts whenever its libc++ side runs.

| Quantity | libstdc++ (canonical) | libc++ |
|---|---:|---:|
| full, switches per encounter | 0.40 +/- 0.49 | 0.40 +/- 0.49 |
| full, max switches in one encounter | 1 | 1 |
| immediate argmin, switches per encounter | 29.74 +/- 36.70 | 30.22 +/- 37.44 |
| immediate argmin, `near_tie` switches | 98.16 +/- 7.85 | 100.28 +/- 6.45 |
| synthetic correspondence loss, switches per encounter | 37.20 +/- 36.12 | 37.24 +/- 36.35 |
| synthetic correspondence loss, censored | 71/250 | 65/250 |
| simple dwell, `near_tie` switches | 0.00 +/- 0.00 | 0.02 +/- 0.14 |
| R5 full `clean_commit` mean `t_sfx`, v_lat 0.05 / 0.10 / 0.20 | 2.37 / 2.40 / 2.57 | 2.40 / 2.42 / 2.59 |

The same substitution, applied to the historical commit `9fe495a`, is the one an
external rerun reported and the one reconstructed in
[`docs/reviews/omnilink-reproduction-2026-09-14.md`](../../../../docs/reviews/omnilink-reproduction-2026-09-14.md).

## Reproduce

```bash
# g++/libstdc++ must reproduce ../ablation_raw.csv and ../R5_rho_sweep.md; clang++/libc++ must reproduce this directory.
REQUIRE_LIBCXX=1 bash scripts/check_stdlib_sensitivity.sh
# A non-system libc++ (here conda-forge clangxx + libcxx):
ENV=/path/to/conda-env   # contains bin/clang++ and include/c++/v1
CXX_LIBCXX=$ENV/bin/clang++ \
LIBCXX_FLAGS="-nostdinc++ -isystem $ENV/include/c++/v1 -L$ENV/lib -Wl,-rpath,$ENV/lib" \
REQUIRE_LIBCXX=1 bash scripts/check_stdlib_sensitivity.sh
```

Recorded on 2026-09-30 from branch `repro/stdlib-sensitivity` (base `6ea9ac3`):
clang 23.1.2 with LLVM libc++ (`_LIBCPP_VERSION` 230102, conda-forge), Ubuntu
20.04 x86_64, `-std=c++17 -O2`. The libstdc++ side was GCC 9.4.0 on the same
machine and reproduced `../ablation_raw.csv` and `../R5_rho_sweep.md` byte for byte.
conda-forge clang 18.1.8 with libc++ 18.1.8 reproduced this directory byte for byte as well.
The CI step (ubuntu-24.04, Ubuntu libc++-18-dev 1:18.1.3-1ubuntu1,
`REQUIRE_LIBCXX=1`) passed the check at commit `1aa7176` (GitHub Actions run
36646717310). Only these toolchains were compared; the result does not
establish identity for any other standard library, compiler version or platform.
