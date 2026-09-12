#!/usr/bin/env python3
"""Record a generated manuscript snapshot; integrity checking is a separate gate."""
import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--write', action='store_true', required=True)
    parser.parse_args()
    source = ROOT / 'paper/arxiv/main.tex'
    paths = [source, source.with_suffix('.pdf')]
    for name in re.findall(r'\\includegraphics(?:\[[^\]]*\])?\{([^}]+)\}', source.read_text()):
        paths.append(source.parent / name)
    data = {'schema_version': 1, 'artifacts': {
        p.relative_to(ROOT).as_posix(): {'sha256': hashlib.sha256(p.read_bytes()).hexdigest()}
        for p in sorted(set(paths))}}
    (source.parent / 'artifact_manifest.json').write_text(json.dumps(data, indent=2) + '\n')

if __name__ == '__main__':
    main()
