#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
core=(src/compass_core/src/*.cpp)
inc=(-I src/compass_core/include -I src/compass_eval/include)
g++ -std=c++17 -O2 "${inc[@]}" "${core[@]}" src/compass_eval/test_response.cpp -o "$out/test"
"$out/test"
g++ -std=c++17 -O2 "${inc[@]}" "${core[@]}" src/compass_eval/src/trace_main.cpp -o "$out/trace"
python3 scripts/run_response_comparison.py "$out/trace" "${1:-/tmp/compass_response_results}"
python3 -m unittest discover -s scripts -p test_physical_metrics.py
