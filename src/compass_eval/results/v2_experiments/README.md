# v2 experiments (post hoc, added after review)

Plan, fixed before the first run: `paper/arxiv_ref/V2_EXPERIMENT_PLAN.md`
(commit `141c993`). Code: `src/compass_eval/include/compass_eval/harness_v2.hpp`
and `src/compass_eval/src/v2_main.cpp`. Archived results (`../ablation_raw.csv`
and the other files in the parent directory) are not changed.

```bash
g++ -std=c++17 -O2 -I src/compass_core/include -I src/compass_eval/include \
    src/compass_core/src/*.cpp src/compass_eval/src/v2_main.cpp -o /tmp/compass_eval_v2
mkdir -p /tmp/v2 && /tmp/compass_eval_v2 all /tmp/v2
bash scripts/test_v2_experiments.sh        # rebuild, byte comparison, checks
python3 paper/arxiv_ref/tools/gen_v2_tables.py --write   # tables, V2_results.md
```

Files: `tuning_grid.csv` (every grid point, tuning and evaluation seeds),
`selected_params.csv`, `v2_native_raw.csv` (native noise, 11 policies),
`v2_sigma_raw.csv` (uniform noise sweep), `v2_late_reversal_raw.csv`,
`V2_results.md` (generated summary). Results are libstdc++ (g++) results; no
libc++ rerun was made. Scripted targets are properties of the scenario scripts,
not independent labels.
