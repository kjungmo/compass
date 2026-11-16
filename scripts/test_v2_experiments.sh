#!/usr/bin/env bash
# Rebuild the experiments added in v2 after review (post hoc) and compare every
# output byte for byte with the committed files. libstdc++ (g++) only.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
cd "$root"
core=(src/compass_core/src/*.cpp)
inc=(-I src/compass_core/include -I src/compass_eval/include)
g++ -std=c++17 -O2 "${inc[@]}" "${core[@]}" src/compass_eval/src/v2_main.cpp -o "$out/v2"
mkdir "$out/res"
"$out/v2" all "$out/res"
res=src/compass_eval/results/v2_experiments
for f in tuning_grid.csv selected_params.csv v2_native_raw.csv v2_sigma_raw.csv v2_late_reversal_raw.csv; do
  cmp "$out/res/$f" "$res/$f"
done
echo "v2 raw outputs equal the committed files"
python3 scripts/check_v2_experiments.py
