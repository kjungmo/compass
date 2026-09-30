# Historical OmniLink rerun: independent reconciliation, 2026-09-14

## Scope and disposition

This note concerns only `9fe495a7f12e06cd2a3da46d0a3aa7f1db92859f`.
It does not rerun the newer 0.30 s observer, responsive profile, measured-progress
adapter or candidate Nav2 execution path. It supplements the prior completion
checkpoint; it does not close any Gazebo/robot, independent-oracle or human-study gate.

The distribution-substitution explanation for the historical discrepancy is now
independently corroborated on Ubuntu GCC 13.3.0. The external packaging is still
incomplete, the original Zig build was not recreated, and no general
cross-toolchain floating-point identity is established.

The 2026-09-30 addendum at the end of this note supersedes two statements
below: four of the five remaining packaging issues are resolved in OmniLink's
v2 package (issue 5 stands), and the libc++ result is now also confirmed by a
native LLVM libc++ build.

## Method

The eight translation units and eleven headers were retrieved at the exact
historical commit through GitHub. Their raw SHA-256 values all match OmniLink's
19-entry source manifest. The historical repository CSV was fetched as raw
base64 bytes, decoded and compared directly to the local output.

The diagnostic header was reconstructed from OmniLink's supplied unified diff
after normalizing the diff's line endings. Upstream source files were not
modified. The reconstructed header was placed first on the include path.
All builds used the same historical sources and 50 seeds per scenario.

- Baseline: `g++ -std=c++17`, no optimization flag.
- Optimization comparison: add `-O3 -ffast-math -march=native`.
- Diagnostic probe: baseline flags plus `-DOMNILINK_COIN_MODE=1` and the probe
  include path before the original evaluation include directory.
- Core source set: `src/compass_core/src/*.cpp`; driver:
  `src/compass_eval/src/main.cpp`.
- Invocations: `eval ablation OUTPUT_DIR` and `eval rho_sweep`.
- Compiler: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0`.

This is a GCC diagnostic emulation using the supplied replacement algorithms,
not a native libc++ or Zig execution.

## Independent findings

All baseline CSV bytes equal the historical repository Git blob. Baseline and
optimization-comparison outputs are identical. The probe differs in exactly
336 of 1,500 parsed ablation rows.

| Mean switches per encounter | Baseline | Diagnostic probe |
|---|---:|---:|
| Full | 0.400 | 0.400 |
| Minus hysteresis | 0.400 | 0.400 |
| Minus progress hardening | 0.400 | 0.400 |
| Immediate argmin | 29.744 | 30.224 |
| Simple dwell | 0.800 | 0.804 |
| Synthetic label-flicker comparator | 37.200 | 37.244 |

Simple dwell is equal only after rounding to two decimal places.

A separately written per-seed exporter called the historical
`make_scenario(CleanCommit, seed, v_lat)`, `run_variant(Full, ...)` and
`compute(...)` for seeds 0 through 49. It produced 250 rows per implementation;
91 rows differ. This agrees with the reported count, but the external exporter
and CSV bytes were not supplied, so identity with those missing files is not claimed.

| v_lat (m/s) | Switched seeds, both builds | Baseline mean t_legible (s) | Probe mean t_legible (s) |
|---|---:|---:|---:|
| 0.05 | 50/50 | 2.37 | 2.40 |
| 0.10 | 50/50 | 2.40 | 2.42 |
| 0.20 | 50/50 | 2.57 | 2.59 |
| 0.35 | 0/50 | 0.05 | 0.05 |
| 0.50 | 0/50 | 0.05 | 0.05 |

These are historical observer outputs. The 0.05 s value in blocked cases denotes
a constant label, not warranted responsiveness or human-perceived motion legibility.

## Serialization and hashes

OmniLink's reported CSV and rho-summary hashes correspond to CRLF serialization.
Linux emits LF here. Converting LF outputs to CRLF reproduces every corresponding
SHA-256 and MD5 listed below; the source computation is unchanged.

| CSV | Raw Linux LF SHA-256 | CRLF SHA-256 matching external manifest |
|---|---|---|
| Baseline / fast | `2a37f94cfbd3eb7491dac5c0b0fa7923d668125ec5bd1d5af29e2a34009442d2` | `05e1bec7de9ec878748bd1d9e59f57b90551c31f962797ce8d649a591760224b` |
| Probe | `2cce8c729bf2027dca3382d303c1e80dc8dbf6dc17ce0f50216b3a0f82d7e709` | `f3dadce5506575bbc5bbd4f97bac0a58f3f7cad849924ace64e5c34345c02a71` |

CRLF MD5: baseline/fast `04392af830963e691df2c10927179825`;
probe `e3a91544f26c77ae5a3060c062c80c46`.

The historical repository Git blob is the LF baseline above. Describing its
CRLF representation as the repository's raw byte hash is inaccurate.
This is separate from whether line endings alter compiled program behavior.

Rho-summary SHA-256:
- Baseline LF: `566de51bab805d0d6677b4ba55283c36aae979baecabf9fedd1f7fd67a0bceac`.
- Baseline CRLF: `5c94a8ea3b87d2aec75b437585dad291e3bf84ff8d5d0d74f70a7e0f07769202`.
- Probe LF: `e6855166e8fefcec672abed37ced963fd9280a6c76e9f315b98e4cdf120885ca`.
- Probe CRLF: `a9f47a7ad5cef032e060ebc7ffeb03e1275df1dc7cb86637d70554d8192477c0`.

The original September 8 attachment's raw download was blocked by HTTP 403 in
this audit environment. Agreement with that attachment is established here
against the sender's reported hash, not a newly downloaded original byte stream.

## Remaining external package issues

1. `reproduce.sh` expects a probe header but neither ships nor creates it.
   Missing probe include directories can silently select the original header.
2. The script writes `_v.cpp` with the library header, then preprocesses
   `/dev/null` instead. It does not collect the intended library macros.
   A correct probe is:
   `g++ -std=c++17 -dM -E -x c++ -include bits/c++config.h /dev/null`.
   Record the runtime library package/version too.
3. `rho_sweep` outputs aggregate tables. The listed `rho_dump.cpp` and
   per-seed CSVs are absent from the supplied three-file package.
4. Expected hashes are printed rather than asserted; failures can be followed
   by a successful script exit. Commit identity, input cleanliness and fresh
   output directories should be enforced. Paths with spaces and relative output
   paths also need reliable handling.
5. Build 1 is explicitly reconstructed and not run by the supplied script.
   Fresh builds 2–4 do not recover the original September 8 command log.

These packaging issues do not negate the independent numeric reproduction.
The author reply sent on 2026-09-14 requests a complete, fail-closed package
and acknowledges the independently reproduced mechanism.

## Paper and repository incorporation

| Destination | Permitted update | Boundary |
|---|---|---|
| Reproducibility appendix | Historical attribution, source/toolchain identity, distribution sensitivity, LF/CRLF rules | No platform-universal deterministic-sampler claim |
| Current evaluation tables | Keep current observer/data provenance | Do not replace current tables with this old observer's values |
| Methods / comparator description | Synthetic label-flicker comparator; immediate argmin distinguished from core knob ablations | Do not call either an isolated correspondence removal |
| Limitations | Offline transition blocking remains distinct from physical freezing | No motion-legibility, Gazebo/robot or goal-success claim |
| Future dataset format | Store generated fixed inputs plus hashes; version sampler changes | Seed plus generator identity is not proof of identical realized inputs on every toolchain |

### Proposed manuscript paragraph (not yet inserted into the canonical PDF)

An external rerun by OmniLink examined the historical offline decision-core
harness at commit 9fe495a7f12e. An independent GCC 13.3 reconstruction reproduced
the 336 differing ablation rows after substituting the supplied distribution
mappings, while the original harness reproduced the archived repository CSV.
The external output hashes were recovered after accounting for LF/CRLF
serialization. This result corroborates distribution-dependent input generation
in that historical harness; it does not establish cross-toolchain numerical
identity, validate subsequent observer or controller changes, or provide
closed-loop physical performance evidence.

The canonical manuscript/PDF and current evaluation datasets are unchanged by
this documentation-only addition. Inserting the paragraph later requires the
usual manuscript rebuild and consistency checks. Exact-head CI must be
rechecked before merge; earlier green checks refer to their own commit.

## Addendum 2026-09-30: v2 package and native libc++

### OmniLink v2 package

OmniLink replied on 2026-09-29 with `compass_reconciliation_v2_2026-09-29.zip`
(15,652 bytes, SHA-256
`64e1b4a51edad8d815e1dc2eb7e6470a4e0afdd132c3d50e2145cd5276dc29ce`, equal to the
value stated in their message). The package was read in full before it was run.
It builds and runs the historical sources only inside its output directory and
does not access the network.

It was run against a clone containing `9fe495a` with GCC 9.4.0 on Ubuntu 20.04
x86_64. This is a third libstdc++ toolchain, after OmniLink's MinGW g++ 15.2 and
g++ 11.4. Runs with an absolute output path containing spaces and with a
relative output path containing spaces both ended in `PASS`. All 20 source
hashes, the probe-header hash and its byte equality with the repository header
plus `substitution.diff`, and all 11 output hashes matched. The b2 LF output
equals the repository blob byte for byte.

Nine failure paths were checked, and each stopped with `FAIL` and a non-zero
exit: a non-empty output directory; a repository without the commit; a tampered
probe header; a missing probe header; a diff inconsistent with the header; a
source-hash mismatch; a missing compiler; the probe directory dropped from the
build-4 include check; and a changed expected output hash. The last two were
added here and were not among OmniLink's own checks.

Cross-checks against the 2026-09-14 reconstruction all agree. There are 336 of
1,500 differing ablation rows and 91 of 250 differing per-seed rho rows. The R5
means are 2.37/2.40/2.57 s against 2.40/2.42/2.59 s. The shipped reference
per-seed CSVs are byte-identical to the regenerated ones.

The v2 package resolves the first four remaining issues listed above:

1. The probe header ships with the package and is checked three ways.
2. The macros come from a real translation unit, and the runtime libraries are
   recorded.
3. `rho_dump.cpp` and the per-seed CSVs are included.
4. Hashes are asserted, the sources are exported from the pinned commit, the
   output directory must be fresh, and paths with spaces work.

Issue 5 stands. Build 1 remains a labelled reconstruction, and the 2026-09-08
command log is not recovered.

### Native LLVM libc++

The emulation caveat above ("not a native libc++ or Zig execution") is now
closed for libc++. The unmodified `9fe495a` sources were built with clang 23.1.2
against LLVM libc++ (`_LIBCPP_VERSION` 230102, conda-forge, Linux x86_64,
`-std=c++17 -O2`). The resulting ablation CSV
(`2cce8c729bf2027dca3382d303c1e80dc8dbf6dc17ce0f50216b3a0f82d7e709`) and rho
summary (`e6855166e8fefcec672abed37ced963fd9280a6c76e9f315b98e4cdf120885ca`)
equal the LF-normalised hashes of OmniLink's libc++ output. This is independent
native evidence for the distribution-substitution explanation. It is still a
Linux libc++ result: the original zig/Windows binary was not rerun.

### Current harness

The same dependence holds at the current head, because the harness still draws
scenario noise from `std::normal_distribution` and the synthetic comparator's
label from `std::uniform_int_distribution`. A native libc++ build of the
unchanged current harness changes 425 of 1,500 rows. The canonical libstdc++
switch counts for the proposed method and both knob-only ablations are
unchanged in all 750 rows. The record and a fail-closed check are in
[`src/compass_eval/results/stdlib_sensitivity/`](../../src/compass_eval/results/stdlib_sensitivity/README.md)
and `scripts/check_stdlib_sensitivity.sh`.

The manuscript now states this dependence, using the current-harness values, in
the Experiments section (§5 of the canonical `paper/arxiv/main.tex`, §4 of the
re-typeset `paper/arxiv_ref`) and in the Reproduction note (§5.3 of `main.tex`;
Appendix A, Reproducibility, of `arxiv_ref`). It does not use the historical
`9fe495a` values. The proposed paragraph above is superseded and is not
inserted.
