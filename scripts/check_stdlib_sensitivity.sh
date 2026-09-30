#!/usr/bin/env bash
# Standard-library sensitivity of the offline harness (docs/reviews/omnilink-reproduction-2026-09-14.md).
#
# The scenario generator draws from std::normal_distribution and the synthetic
# comparator from std::uniform_int_distribution. Their algorithms are
# implementation-defined, so the realized inputs depend on the C++ standard
# library. This script builds the unchanged harness twice:
#   - g++ / libstdc++      -> must equal the canonical results/ablation_raw.csv
#   - clang++ / LLVM libc++ -> must equal results/stdlib_sensitivity/
# and asserts the row counts the manuscript prints (425 of the 1,500 ablation
# rows differ; the proposed method and knob-only ablations keep their switch,
# sign-change and entropy columns).
#
#   scripts/check_stdlib_sensitivity.sh           verify (skips libc++ if unavailable)
#   scripts/check_stdlib_sensitivity.sh --write   regenerate results/stdlib_sensitivity/ (needs libc++)
#
# CXX (default g++) and CXX_LIBCXX (default clang++) select the compilers;
# LIBCXX_FLAGS adds flags for a non-system libc++ (e.g. -nostdinc++ -isystem ...).
# REQUIRE_LIBCXX=1 turns a missing libc++ toolchain into a failure (always so with --write).
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
mode=${1:-check}
[ "$mode" = check ] || [ "$mode" = --write ] || { echo "usage: $0 [--write]" >&2; exit 2; }
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
res=src/compass_eval/results
dst=$res/stdlib_sensitivity
core=(src/compass_core/src/*.cpp)
inc=(-I src/compass_core/include -I src/compass_eval/include)
read -r -a libcxx_flags <<< "${LIBCXX_FLAGS:-}"

# Run ablation and rho_sweep; drop the machine-dependent CPU line from the R1 summary.
# The libc++ summary points at the CSV name it is stored under in stdlib_sensitivity/.
run() {
  local csvname=ablation_raw.csv
  [ "$1" = libcxx ] && csvname=ablation_raw_libcxx.csv
  mkdir "$out/$1"
  (cd "$out/$1" && ../bin_"$1" ablation . | grep -v '^CPU: ' \
    | sed "s#^raw CSV -> \./ablation_raw\.csv\$#raw CSV -> ./$csvname#" > R1_ablation.md \
    && ../bin_"$1" rho_sweep > R5_rho_sweep.md)
}

"${CXX:-g++}" -std=c++17 -O2 "${inc[@]}" "${core[@]}" src/compass_eval/src/main.cpp -o "$out/bin_stdcxx"
run stdcxx
cmp "$out/stdcxx/ablation_raw.csv" "$res/ablation_raw.csv"
cmp "$out/stdcxx/R5_rho_sweep.md" "$res/R5_rho_sweep.md"
echo "libstdc++: ablation_raw.csv and R5_rho_sweep.md equal the canonical files"

cxx=${CXX_LIBCXX:-clang++}
macros=$(printf '#include <cstddef>\n' \
  | "$cxx" -std=c++17 -stdlib=libc++ "${libcxx_flags[@]}" -x c++ -dM -E - 2>/dev/null || true)
if ! grep -q '_LIBCPP_VERSION' <<< "$macros"; then
  if [ "${REQUIRE_LIBCXX:-0}" = 1 ] || [ "$mode" = --write ]; then echo "FAIL: no libc++ toolchain ($cxx)" >&2; exit 1; fi
  echo "SKIP: $cxx cannot compile against libc++; set CXX_LIBCXX/LIBCXX_FLAGS to check"
  exit 0
fi
"$cxx" -std=c++17 -O2 -stdlib=libc++ "${libcxx_flags[@]}" "${inc[@]}" "${core[@]}" \
  src/compass_eval/src/main.cpp -o "$out/bin_libcxx"
run libcxx

if [ "$mode" = --write ]; then
  mkdir -p "$dst"
  cp "$out/libcxx/ablation_raw.csv" "$dst/ablation_raw_libcxx.csv"
  cp "$out/libcxx/R1_ablation.md" "$dst/R1_ablation_libcxx.md"
  cp "$out/libcxx/R5_rho_sweep.md" "$dst/R5_rho_sweep_libcxx.md"
  echo "wrote $dst; update $res/PROVENANCE.md"
else
  cmp "$out/libcxx/ablation_raw.csv" "$dst/ablation_raw_libcxx.csv"
  cmp "$out/libcxx/R1_ablation.md" "$dst/R1_ablation_libcxx.md"
  cmp "$out/libcxx/R5_rho_sweep.md" "$dst/R5_rho_sweep_libcxx.md"
  echo "libc++: outputs equal $dst"
fi

# Fail-closed checks of the libc++ claims printed in the manuscript (Python 3.8 compatible).
python3 - "$res/ablation_raw.csv" "$out/libcxx/ablation_raw.csv" <<'PY'
import csv, sys

# Stated in src/compass_eval/results/stdlib_sensitivity/README.md ("425 of the 1,500 rows differ").
EXPECTED_DIFF_ROWS = 425
CORE = ["full (제안)", "-hysteresis", "-progress hardening"]
# Rows whose switch count differs, per variant (same README, "What changes").
EXPECTED_SWITCH_DIFF = {"full (제안)": 0, "-hysteresis": 0, "-progress hardening": 0,
                        "-accumulator (즉시 argmin)": 120, "-class correspondence": 132,
                        "simple-dwell (O4)": 1}
COLS = ["switches", "sign_change_rate", "decision_entropy"]


def require(ok, msg):
    # Explicit exit rather than assert, so PYTHONOPTIMIZE cannot disable the checks.
    if not ok:
        sys.exit("FAIL: " + msg)


def load(path):
    with open(path, encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        return reader.fieldnames, list(reader)


(ha, a), (hb, b) = load(sys.argv[1]), load(sys.argv[2])
require(ha == hb and len(a) == len(b), "CSV shape differs")
key = ("variant", "scenario", "seed")
require(all(tuple(x[k] for k in key) == tuple(y[k] for k in key) for x, y in zip(a, b)), "row order differs")
variants = []
for r in a:
    if r["variant"] not in variants:
        variants.append(r["variant"])
require(sorted(variants) == sorted(EXPECTED_SWITCH_DIFF), f"unexpected variants: {variants}")

diff = {v: 0 for v in variants}
sw_diff = {v: 0 for v in variants}
for x, y in zip(a, b):
    diff[x["variant"]] += x != y
    sw_diff[x["variant"]] += x["switches"] != y["switches"]
total = sum(diff.values())
print(f"rows differing between libstdc++ and libc++: {total}/{len(a)}")
print("  per variant: " + ", ".join(f"{v} {diff[v]}" for v in variants))
require(total == EXPECTED_DIFF_ROWS, f"expected {EXPECTED_DIFF_ROWS} differing rows, got {total}")

core = [(x, y) for x, y in zip(a, b) if x["variant"] in CORE]
for x, y in core:
    for c in COLS:
        require(x[c] == y[c], f"core {c} differs: {x['variant']}/{x['scenario']}/{x['seed']}")
max_sw = max(max(int(x["switches"]), int(y["switches"])) for x, y in core)
require(max_sw <= 1, f"core variant exceeds one switch: {max_sw}")
print(f"core variants (full, -hysteresis, -progress hardening): switches/sign_change_rate/decision_entropy "
      f"identical in {len(core)}/{len(core)} rows; max switches {max_sw}")

print("rows whose switches differ: " + ", ".join(f"{v} {sw_diff[v]}" for v in variants))
require(sw_diff == EXPECTED_SWITCH_DIFF, f"switch-differing rows {sw_diff} != {EXPECTED_SWITCH_DIFF}")
PY
