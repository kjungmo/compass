#!/usr/bin/env python3
"""Offline opportunity scoring. Oracle labels are supplied separately, never inferred from J.

Trace JSONL: cycle,t,dt,class,safety. Optional invalid=true marks data loss.
Oracle JSONL: id,onset,confirmed,deadline,expiry,target,hold,lock_window.
All times are seconds in the same simulation clock. Only complete observations count.
"""
import argparse
import json
import math
from collections import Counter


def key(value):
    pairs = tuple(sorted((int(p[0]), int(p[1])) for p in value))
    if len({p[0] for p in pairs}) != len(pairs) or any(p[1] not in (0, 1) for p in pairs):
        raise ValueError('invalid class key')
    return pairs


def score(rows, opportunities, end_time, ending='timeout'):
    if ending not in ('goal', 'collision', 'timeout', 'aborted', 'invalid_log'):
        raise ValueError('unknown ending')
    if not math.isfinite(end_time) or end_time < 0:
        raise ValueError('invalid end_time')
    seen = set()
    for op in opportunities:
        if op['id'] in seen:
            raise ValueError('duplicate opportunity id')
        seen.add(op['id'])
        times = [op[k] for k in ('onset', 'confirmed', 'deadline', 'expiry', 'hold', 'lock_window')]
        if not all(math.isfinite(x) for x in times):
            raise ValueError('nonfinite oracle value')
        if not (0 <= op['onset'] <= op['confirmed'] <= op['deadline'] and op['confirmed'] <= op['expiry']):
            raise ValueError('oracle times must be ordered')
        if op['hold'] <= 0 or op['lock_window'] <= 0:
            raise ValueError('durations must be positive')
        key(op['target'])
    invalid = ending == 'invalid_log' or not rows
    previous = None
    for r in rows:
        if not all(math.isfinite(r[k]) for k in ('t', 'dt')) or r['dt'] <= 0 or r['t'] < 0:
            raise ValueError('invalid trace time')
        key(r['class'])
        if not isinstance(r['safety'], bool):
            raise ValueError('safety must be boolean')
        if previous and (r['cycle'] != previous['cycle'] + 1 or
                         abs(r['t'] - previous['t'] - previous['dt']) > 1e-8):
            invalid = True
        invalid |= bool(r.get('invalid', False))
        previous = r
    if rows and (rows[0]['cycle'] != 0 or abs(rows[0]['t']) > 1e-8 or
                 abs(rows[-1]['t'] + rows[-1]['dt'] - end_time) > 1e-8):
        invalid = True
    results = []
    for op in opportunities:
        target = key(op['target'])
        limit = min(end_time, op['expiry'])
        hold_start = None
        completed = None
        safety_time = None
        for r in rows:
            if r['t'] < op['onset'] or r['t'] >= limit:
                continue
            if r['safety']:
                safety_time = r['t']
                break
            if key(r['class']) == target:
                if hold_start is None:
                    hold_start = r['t']
                # Conservative: hold is confirmed only by a later sample, not extrapolation.
                if r['t'] - hold_start + 1e-9 >= op['hold'] and r['t'] >= op['confirmed']:
                    completed = r['t']
                    break
            else:
                hold_start = None
        eligible = True
        if invalid:
            outcome, eligible = 'invalid_log', False
        elif completed is not None:
            outcome = 'timely' if completed <= op['deadline'] + 1e-9 else 'late'
        elif safety_time is not None:
            outcome, eligible = 'safety_interrupted', safety_time > op['deadline']
        elif ending in ('collision', 'aborted') and end_time <= op['expiry']:
            outcome = 'competing_failure'
        elif op['expiry'] < op['deadline'] and end_time >= op['expiry']:
            outcome, eligible = 'opportunity_expired', False
        elif end_time < op['deadline']:
            outcome, eligible = 'insufficient_followup', False
        else:
            outcome = 'missed'
        if rows:
            before = next((r for r in reversed(rows) if r['t'] < op['onset']), None)
            initial = before['class'] if before else rows[0].get('before', rows[0]['class'])
            if key(initial) == target and not invalid:
                outcome, eligible = 'already_target', False
        lock_end = op['confirmed'] + op['lock_window']
        lock_observed = not invalid and limit >= lock_end and (safety_time is None or safety_time > lock_end)
        results.append(dict(id=op['id'], outcome=outcome, eligible=eligible,
                            response_delay_s=(hold_start-op['onset']) if completed is not None and not invalid else None,
                            confirmation_time=completed if not invalid else None,
                            observed_until=limit,
                            persistent_lockup=(completed is None or completed > lock_end)
                              if lock_observed and outcome != 'already_target' else None))
    counts = Counter(r['outcome'] for r in results)
    n = sum(r['eligible'] for r in results)
    return dict(ending=ending, valid_log=not invalid, opportunities=results,
                counts=dict(counts), eligible_opportunities=n,
                warranted_switch_recall=counts['timely']/n if n else None)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace'); parser.add_argument('oracle')
    parser.add_argument('--end-time', type=float, required=True)
    parser.add_argument('--ending', default='timeout')
    args = parser.parse_args()
    def read(path):
        with open(path, encoding='utf-8') as f:
            return [json.loads(line) for line in f if line.strip()]
    print(json.dumps(score(read(args.trace), read(args.oracle), args.end_time, args.ending),
                     indent=2, allow_nan=False))


if __name__ == '__main__':
    main()
