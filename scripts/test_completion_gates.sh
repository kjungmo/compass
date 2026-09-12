#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
core=(src/compass_core/src/*.cpp)
inc=(-I src/compass_core/include -I src/compass_eval/include -I src/compass_nav2/include)
g++ -std=c++17 -O2 "${inc[@]}" "${core[@]}" src/compass_nav2/src/path_tracking.cpp src/compass_nav2/test/test_candidate_cost.cpp -o "$out/candidate"
"$out/candidate"
g++ -std=c++17 -O2 "${inc[@]}" src/compass_core/src/topo_class.cpp src/compass_nav2/test/test_measured_progress.cpp -o "$out/progress"
"$out/progress"
g++ -std=c++17 -O2 "${inc[@]}" "${core[@]}" src/compass_nav2/test/test_velocity_limits.cpp -o "$out/velocity"
"$out/velocity"
g++ -std=c++17 -O2 "${inc[@]}" "${core[@]}" src/compass_nav2/src/path_tracking.cpp src/compass_eval/src/closed_loop_diagnostic.cpp -o "$out/closed_loop"
result_dir=${1:-/tmp/compass_closed_loop}
mkdir -p "$result_dir"
for profile in legacy_proxy legacy_measured responsive_measured; do
  "$out/closed_loop" "$profile" "$result_dir/$profile.jsonl"
done
python3 - "$result_dir" <<'PY'
import hashlib,json,sys
from pathlib import Path
sys.path.insert(0,'scripts')
from physical_metrics import score_physical
p=Path(sys.argv[1]);result={}
for profile in ('legacy_proxy','legacy_measured','responsive_measured'):
    raw=(p/(profile+'.jsonl')).read_bytes();r=[json.loads(x) for x in raw.splitlines()]
    ending='collision' if r[-1]['collision'] else 'goal' if r[-1]['goal_reached'] else 'timeout'
    s=score_physical(r,ending,60);s['switches']=sum(x['switched'] for x in r);s['trace_sha256']=hashlib.sha256(raw).hexdigest();result[profile]=s
assert result['legacy_proxy']['ending']=='timeout'
assert result['legacy_measured']['success'] and result['responsive_measured']['success']
(p/'summary.json').write_text(json.dumps(dict(scope='scripted-cost unicycle diagnostic; not Nav2/Gazebo physical validation',results=result),indent=2)+'\n')
print((p/'summary.json').read_text())
PY
python3 -m unittest discover -s scripts -p 'test_*metrics.py'
python3 -m unittest discover -s scripts -p test_evaluate_opportunities.py
