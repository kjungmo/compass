#!/usr/bin/env python3
"""Write a 480-case experiment manifest; this does not run a simulator."""
import itertools,json,sys
scenarios=['head_on_narrow','head_on_wide','narrow_to_wide','wide_to_narrow',
           'human_overtakes','robot_overtakes','late_side_reversal','safe_wait_then_clear']
profiles=['legacy_proxy','legacy_measured','responsive_measured']
cases=[dict(id=f'{s}-{seed:02d}-{p}',scenario=s,seed=seed,profile=p,horizon_s=60.,
            status='pending',geometry_fixture=None,actor_trace_sha256=None,
            oracle_sha256=None,source_sha=None,config_sha256=None)
       for s,seed,p in itertools.product(scenarios,range(20),profiles)]
with open(sys.argv[1],'w') as f:json.dump(dict(schema=1,planned_cases=len(cases),cases=cases),f,indent=2)
