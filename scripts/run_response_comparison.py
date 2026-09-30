#!/usr/bin/env python3
"""Offline paired sweep; no physical performance is inferred."""
import argparse,csv,hashlib,json,subprocess,tempfile
from pathlib import Path
from evaluate_opportunities import score
p=argparse.ArgumentParser();p.add_argument('trace_binary');p.add_argument('output');a=p.parse_args()
out=Path(a.output);out.mkdir(parents=True,exist_ok=True)
rows=[]
with tempfile.TemporaryDirectory() as tmp:
    path=Path(tmp)/'trace.jsonl'
    for scenario in ('near_tie','transient_spike','mid_reversal','clean_commit','intermittent'):
        for v in (.05,.1,.2,.35,.5):
            for seed in range(50):
                for profile,measured in (('legacy',False),('responsive',False),('responsive',True)):
                    cmd=[a.trace_binary,scenario,str(seed),str(v),str(path),profile]
                    if measured:cmd.append(str(v))
                    subprocess.run(cmd,check=True)
                    raw=path.read_bytes();trace=[json.loads(line) for line in raw.splitlines()]
                    # Warranted target declared ONLY for the scripted clean advantage.
                    oracle=[dict(id='clean',onset=0,confirmed=.5,deadline=3.5,expiry=10,
                                 target=[[7,0]],hold=.3,lock_window=5)] if scenario=='clean_commit' else []
                    result=score(trace,oracle,10)
                    rows.append(dict(scenario=scenario,seed=seed,progress_input=v,
                        profile=profile,progress_mode='measured' if measured else 'legacy_proxy',
                        switches=sum(trace[i]['class']!=trace[i-1]['class'] for i in range(1,len(trace))),
                        recall=result['warranted_switch_recall'],
                        outcome=result['opportunities'][0]['outcome'] if oracle else 'no_oracle',
                        trace_sha256=hashlib.sha256(raw).hexdigest()))
with (out/'paired_offline.csv').open('w') as f:
    w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
summary=[]
for scenario in ('near_tie','transient_spike','mid_reversal','clean_commit','intermittent'):
    for profile,mode in (('legacy','legacy_proxy'),('responsive','legacy_proxy'),('responsive','measured')):
        group=[r for r in rows if r['scenario']==scenario and r['profile']==profile and r['progress_mode']==mode]
        summary.append(dict(scenario=scenario,profile=profile,progress_mode=mode,n=len(group),
                            mean_switches=sum(r['switches'] for r in group)/len(group)))
(out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps(summary,indent=2))
for v in (.05,.1,.2,.35,.5):
    print('clean',v,[(profile,mode,sum(r['recall'] for r in rows if r['scenario']=='clean_commit' and r['progress_input']==v and r['profile']==profile and r['progress_mode']==mode)) for profile,mode in (('legacy','legacy_proxy'),('responsive','legacy_proxy'),('responsive','measured'))])
