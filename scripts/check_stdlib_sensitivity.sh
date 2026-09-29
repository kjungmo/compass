#!/usr/bin/env bash
# Standard-library sensitivity of the offline harness (docs/reviews/omnilink-reproduction-2026-09-14.md).
#
# The scenario generator draws from std::normal_distribution and the synthetic
# comparator from std::uniform_int_distribution. Their algorithms are
# implementation-defined, so the realized inputs depend on the C++ standard
# library. This script builds the unchanged harness twice:
#   - g++ / libstdc++      -> must equal the canonical results/ablation_raw.csv
#   - clang++ / LLVM libc++ -> must equal results/stdlib_sensitivity/
# and reports how many of the 1,500 ablation rows differ.
#
#   scripts/check_stdlib_sensitivity.sh           verify (skips libc++ if unavailable)
#   scripts/check_stdlib_sensitivity.sh --write   regenerate results/stdlib_sensitivity/
#
# CXX (default g++) and CXX_LIBCXX (default clang++) select the compilers;
# LIBCXX_FLAGS adds flags for a non-system libc++ (e.g. -nostdinc++ -isystem ...).
# REQUIRE_LIBCXX=1 turns a missing libc++ toolchain into a failure.
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
run() {
  mkdir "$out/$1"
  (cd "$out/$1" && ../bin_"$1" ablation . | grep -v '^CPU: ' > R1_ablation.md && ../bin_"$1" rho_sweep > R5_rho_sweep.md)
}

"${CXX:-g++}" -std=c++17 -O2 "${inc[@]}" "${core[@]}" src/compass_eval/src/main.cpp -o "$out/bin_stdcxx"
run stdcxx
cmp "$out/stdcxx/ablation_raw.csv" "$res/ablation_raw.csv"
echo "libstdc++: ablation_raw.csv equals the canonical CSV"

cxx=${CXX_LIBCXX:-clang++}
macros=$(printf '#include <cstddef>\n' \
  | "$cxx" -std=c++17 -stdlib=libc++ "${libcxx_flags[@]}" -x c++ -dM -E - 2>/dev/null || true)
if ! grep -q '_LIBCPP_VERSION' <<< "$macros"; then
  [ "${REQUIRE_LIBCXX:-0}" = 1 ] && { echo "FAIL: no libc++ toolchain ($cxx)" >&2; exit 1; }
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

python3 - "$res/ablation_raw.csv" "$out/libcxx/ablation_raw.csv" <<'PY'
import csv, sys
a, b = (list(csv.reader(open(p, encoding="utf-8"))) for p in sys.argv[1:])
assert a[0] == b[0] and len(a) == len(b), "CSV shape differs"
print(f"rows differing between libstdc++ and libc++: {sum(x != y for x, y in zip(a[1:], b[1:]))}/{len(a) - 1}")
PY
