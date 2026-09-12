#!/usr/bin/env python3
"""Check aggregate observer rows against each corresponding raw CSV group."""
import csv
import statistics as st
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=list(csv.DictReader((root/'src/compass_eval/results/ablation_raw.csv').open()))
variants=list(dict.fromkeys(r['variant'] for r in rows))
assert len(rows)==1500 and len(variants)==6
sources=[root/'src/compass_eval/results/R1_ablation.md',root/'paper/paper_draft.md',root/'paper/arxiv/main.tex']
for path in sources:
    lines=[l for l in path.read_text().splitlines() if '/250' in l and (l.startswith('|') or '&' in l)][:6]
    assert len(lines)==6,path
    for variant,line in zip(variants,lines):
        group=[r for r in rows if r['variant']==variant]
        assert len(group)==250
        values=[float(r['t_legible_s']) for r in group]
        mean=f'{st.mean(values):.2f}';sd=f'{st.stdev(values):.2f}'
        count=sum(int(r['censored']) for r in group)
        assert mean in line and sd in line and f'{count}/250' in line,(path,variant,line)
print('GREEN: raw CSV -> R1, Korean draft and English LaTeX')
