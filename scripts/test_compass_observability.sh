#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
cd "$root"
core=(src/compass_core/src/*.cpp)
inc=(-I src/compass_core/include -I src/compass_eval/include)
g++ -std=c++17 -O2 "${inc[@]}" "${core[@]}" src/compass_eval/src/main.cpp -o "$out/eval"
g++ -std=c++17 -O2 "${inc[@]}" "${core[@]}" src/compass_eval/src/trace_main.cpp -o "$out/trace"
g++ -std=c++17 -O2 "${inc[@]}" "${core[@]}" src/compass_eval/test_trace.cpp -o "$out/test"
"$out/test"
"$out/eval" ablation "$out" > "$out/summary.txt"
python3 - "$out" <<'PY'
import csv,sys
from pathlib import Path
out=Path(sys.argv[1])
a=list(csv.DictReader(open('src/compass_eval/results/ablation_raw.csv')))
b=list(csv.DictReader(open(out/'ablation_raw.csv')))
assert a==b, 'canonical CSV mismatch: inspect toolchain and first divergence'
print(f'{len(a)} canonical CSV rows unchanged')
PY
"$out/trace" clean_commit 0 .35 "$out/blocked.jsonl"
"$out/trace" clean_commit 0 .05 "$out/response.jsonl"
python3 - "$out" <<'PY'
import json,sys
from pathlib import Path
sys.path.insert(0,'scripts')
from evaluate_opportunities import score
op=[dict(id='scripted-clean',onset=0,confirmed=.5,deadline=3.5,expiry=10,
         target=[[7,0]],hold=.3,lock_window=5)]
for name,want in [('blocked',0),('response',1)]:
    rows=[json.loads(x) for x in (Path(sys.argv[1])/f'{name}.jsonl').read_text().splitlines()]
    result=score(rows,op,10)
    assert result['warranted_switch_recall']==want
    print(name,result['counts'])
PY
python3 -m unittest discover -s scripts -p test_evaluate_opportunities.py
