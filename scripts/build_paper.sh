#!/usr/bin/env bash
# Build canonical manuscript; --check never modifies tracked outputs.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
mode=${1:---check}
if [[ "$mode" != --check && "$mode" != --write ]]; then
  echo 'usage: bash scripts/build_paper.sh [--check|--write]' >&2
  exit 2
fi
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
cd "$root/paper/arxiv"
# Disable expansion for portability across installed Type 1/font setups.
for pass in 1 2 3; do
  if ! pdflatex -interaction=nonstopmode -halt-on-error -file-line-error \
      -output-directory="$out" -jobname=main \
      '\PassOptionsToPackage{expansion=false}{microtype}\input{main.tex}' > "$out/build.log"; then
    tail -80 "$out/build.log" >&2
    exit 1
  fi
done
if rg -n 'undefined references|undefined citations|LaTeX Warning: (Reference|Citation).*undefined' "$out/main.log"; then
  echo 'unresolved paper references/citations' >&2
  exit 1
fi
pdfinfo "$out/main.pdf" | rg '^Pages:'
if [[ "$mode" == --write ]]; then
  cp "$out/main.pdf" main.pdf
  python3 "$root/scripts/paper_manifest.py" --write
  echo 'Updated paper/arxiv/main.pdf and artifact_manifest.json; visual QA still required.'
else
  echo 'GREEN: canonical LaTeX builds with resolved references (no tracked outputs changed).'
fi
