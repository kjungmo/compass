#!/usr/bin/env python3
"""Score interval summaries from a simulator/robot adapter; never infer motion from labels."""
import argparse,json,math

def score_physical(rows, ending, horizon_s):
    if ending not in ('goal','collision','timeout','aborted') or not rows:
        raise ValueError('missing trace or invalid ending')
    if not math.isfinite(horizon_s) or horizon_s<=0:raise ValueError('invalid horizon')
    prev=None;stop=0.;episodes=0;freeze_s=0.;collision=False
    for r in rows:
        for field in ('t','dt','speed_mps','goal_distance_m','min_clearance_m'):
            if not math.isfinite(r[field]):raise ValueError('nonfinite '+field)
        if r['dt']<=0 or min(r['t'],r['speed_mps'],r['goal_distance_m'])<0:
            raise ValueError('invalid time, speed or distance')
        for field in ('free_motion_warranted','planned_stop','collision','goal_reached'):
            if type(r[field]) is not bool:raise ValueError('invalid boolean '+field)
        if prev is None:
            if r['cycle']!=0 or abs(r['t'])>1e-8:raise ValueError('missing start')
        elif r['cycle']!=prev['cycle']+1 or abs(r['t']-prev['t']-prev['dt'])>1e-8:
            raise ValueError('missing interval')
        collision |= r['collision']
        frozen=(r['speed_mps']<=.02 and r['goal_distance_m']>.3 and
                r['min_clearance_m']>=.5 and r['free_motion_warranted'] and
                not r['planned_stop'] and not r['collision'] and not r['goal_reached'])
        if frozen:stop+=r['dt']
        else:
            if stop>=2.-1e-9:episodes+=1;freeze_s+=stop
            stop=0.
        prev=r
    if stop>=2.-1e-9:episodes+=1;freeze_s+=stop
    elapsed=rows[-1]['t']+rows[-1]['dt']
    if elapsed>horizon_s+1e-8:raise ValueError('trace exceeds horizon')
    if ending=='timeout' and abs(elapsed-horizon_s)>1e-8:raise ValueError('truncated timeout')
    if ending=='goal' and (not rows[-1]['goal_reached'] or rows[-1]['goal_distance_m']>.3 or collision):
        raise ValueError('inconsistent goal termination')
    if ending=='collision' and not collision:raise ValueError('missing collision event')
    return dict(ending=ending,success=ending=='goal',collision=collision,
                freezing_episodes=episodes,freezing_duration_s=freeze_s,
                elapsed_s=elapsed,time_to_goal_s=elapsed if ending=='goal' else None,
                min_clearance_m=min(r['min_clearance_m'] for r in rows))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('trace');p.add_argument('--ending',required=True)
    p.add_argument('--horizon',type=float,required=True);a=p.parse_args()
    with open(a.trace) as f:rows=[json.loads(x) for x in f if x.strip()]
    print(json.dumps(score_physical(rows,a.ending,a.horizon),indent=2,allow_nan=False))
