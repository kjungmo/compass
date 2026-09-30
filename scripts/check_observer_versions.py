#!/usr/bin/env python3
"""Check the archived observer result versions against the per-row comparison.

Issue #3 requires that the saturation-only numerical correction and the later
metric-definition change stay separate and that both result versions remain
available for audit. This gate checks, for the same 1,500 decision streams:

* the per-row comparison CSV has exactly one row per (variant, scenario, seed);
* each version column equals its archived raw CSV (original probability-space
  9fe495a, log-odds-only 5343d45, current 0.30 s follow-up rule);
* switch count, sign-change rate and entropy are identical in all archives, so
  only observer outputs differ;
* the summary table in observer_versions/README.md matches the rows.

`--print-summary` prints the generated Markdown summary table.
Standard library only; failures do not depend on Python assertions.
"""
import csv
import statistics
import sys
from collections import OrderedDict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RESULTS = ROOT / "src" / "compass_eval" / "results"
VERSIONS = RESULTS / "observer_versions"
ROWS = VERSIONS / "observer_versions.csv"
ARCHIVES = OrderedDict((
    ("prob_9fe495a", VERSIONS / "ablation_raw_9fe495a_probability.csv"),
    ("logodds_5343d45", VERSIONS / "ablation_raw_5343d45_logodds_only.csv"),
    ("followup030_current", RESULTS / "ablation_raw.csv"),
))
LABELS = {
    "prob_9fe495a": "9fe495a probability space",
    "logodds_5343d45": "5343d45 log-odds only",
    "followup030_current": "current log-odds + 0.30 s follow-up",
}
BEGIN, END = "<!-- observer-summary:begin -->", "<!-- observer-summary:end -->"


class CheckError(ValueError):
    pass


def require(ok, message):
    if not ok:
        raise CheckError(message)


def key(row):
    return (row["variant"], row["scenario"], row["seed"])


def load(path):
    with path.open(encoding="utf-8", newline="") as source:
        rows = list(csv.DictReader(source))
    require(len(rows) == 1500, f"{path.name}: expected 1500 rows, got {len(rows)}")
    indexed = {key(row): row for row in rows}
    require(len(indexed) == 1500, f"{path.name}: duplicate trial keys")
    return indexed


def summary(rows):
    variants = list(OrderedDict.fromkeys(row["variant"] for row in rows))
    lines = ["| Variant | " + " | ".join(
        f"{LABELS[v]}: mean t_sfx (s) / censored" for v in ARCHIVES) + " |",
             "|---|" + "---|" * len(ARCHIVES)]
    for variant in variants:
        group = [row for row in rows if row["variant"] == variant]
        cells = []
        for version in ARCHIVES:
            times = [float(row["t_sfx_" + version]) for row in group]
            censored = sum(int(row["censored_" + version]) for row in group)
            cells.append(f"{statistics.mean(times):.2f} / {censored}/{len(group)}")
        lines.append(f"| {variant} | " + " | ".join(cells) + " |")
    changed = []
    names = list(ARCHIVES)
    for old, new in zip(names, names[1:]):
        count = sum(1 for row in rows if (row["t_sfx_" + old], row["censored_" + old])
                    != (row["t_sfx_" + new], row["censored_" + new]))
        newly = sum(1 for row in rows if row["censored_" + old] == "0" and row["censored_" + new] == "1")
        released = sum(1 for row in rows if row["censored_" + old] == "1" and row["censored_" + new] == "0")
        changed.append(f"- {old} -> {new}: {count} rows change observer output "
                       f"({released} uncensored, {newly} newly censored).")
    return "\n".join(lines + [""] + changed)


def main(argv):
    try:
        with ROWS.open(encoding="utf-8", newline="") as source:
            rows = list(csv.DictReader(source))
        require(len(rows) == 1500, f"{ROWS.name}: expected 1500 rows, got {len(rows)}")
        require(len({key(row) for row in rows}) == 1500, f"{ROWS.name}: duplicate trial keys")
        archives = {version: load(path) for version, path in ARCHIVES.items()}
        for row in rows:
            k = key(row)
            for version, archive in archives.items():
                require(k in archive, f"{version}: missing trial {k}")
                old = archive[k]
                require(old["switches"] == row["switches"], f"{version}: switches differ at {k}")
                require(old["t_legible_s"] == row["t_sfx_" + version]
                        and old["censored"] == row["censored_" + version],
                        f"{version}: observer output differs from archive at {k}")
        current = archives["followup030_current"]
        for k, row in current.items():
            for version, archive in archives.items():
                for column in ("switches", "sign_change_rate", "decision_entropy"):
                    require(archive[k][column] == row[column],
                            f"{version}: decision-stream column {column} differs at {k}")
        text = summary(rows)
        if "--print-summary" in argv:
            print(text)
            return 0
        readme = (VERSIONS / "README.md").read_text(encoding="utf-8")
        require(BEGIN in readme and END in readme, "observer_versions/README.md: summary markers missing")
        block = readme.split(BEGIN, 1)[1].split(END, 1)[0].strip()
        require(block == text, "observer_versions/README.md: summary does not match rows "
                "(regenerate with --print-summary)")
    except (OSError, KeyError, ValueError) as exc:
        print(f"RED: {exc}", file=sys.stderr)
        return 1
    print("GREEN: 1500 streams; three archived observer versions reproduce row by row; "
          "decision-stream columns identical; summary matches")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
