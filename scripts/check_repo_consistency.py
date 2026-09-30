#!/usr/bin/env python3
"""Bounded, standard-library-only checks of the committed research snapshot.

This checks data/row identity, declared artifact hashes, and known documentation
regressions. It is not a semantic proof of every sentence or a PDF build check.
See docs/reviews/merge-consistency.md for the boundary.
"""

import argparse
import csv
import hashlib
import json
import math
import re
import statistics
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from urllib.parse import unquote, urlsplit


ROOT = Path(__file__).resolve().parents[1]
RESULTS = Path("src/compass_eval/results")
PAPERS = (Path("paper/arxiv/main.tex"), Path("paper/paper_draft.md"))
TABLE_SOURCES = (RESULTS / "R1_ablation.md", *PAPERS)
VARIANTS = ("full", "hysteresis", "hardening", "accumulator", "dwell", "correspondence")
SCENARIOS = ("near_tie", "transient_spike", "mid_reversal", "clean_commit", "intermittent")
METRICS = (("switches", 2), ("sign_change_rate", 3),
           ("decision_entropy", 3), ("t_legible_s", 2))
READMES = tuple(map(Path, ("README.md", "src/README.md", "sim/README.md",
                           "src/compass_eval/results/README.md")))


class CheckError(ValueError):
    """An actionable mismatch, including malformed input, never a Python assert."""


def require(condition, message):
    if not condition:
        raise CheckError(message)


def plain(text):
    """Normalize presentation syntax, not the numbers or table positions."""
    text = text.replace(r"\_", "_").replace(r"\pm", "±")
    text = re.sub(r"\\(?:textbf|textit|mathrm|code)\{([^{}]*)\}", r"\1", text)
    return text.replace("$", "").replace("**", "").replace("`", "").strip().rstrip("\\").strip()


def variant_key(label):
    label = plain(label).lower().lstrip("−–- ")
    for key, pattern in (
        ("full", r"^(?:full\b|전체\s*\(|제안\s*\()"),
        ("hysteresis", r"^(?:hysteresis\b|히스테리시스)"),
        ("hardening", r"^(?:progress\s+hardening\b|진행\s*경화)"),
        ("accumulator", r"^(?:accumulator\b|누적기)"),
        ("dwell", r"^(?:simple[- ]dwell\b|단순\s*드웰)"),
        ("correspondence", r"^class\s*(?:correspondence\b|대응)"),
    ):
        if re.search(pattern, label):
            return key
    return None


def tables(text, latex=False):
    """Parse the simple pipe/tabular tables used by the committed manuscripts."""
    if latex:
        blocks = re.findall(r"\\begin\{tabular\}\{[^\n]*\}(.*?)\\end\{tabular\}", text, re.S)
        return [[list(map(plain, line.split("&"))) for line in block.splitlines()
                 if "&" in line] for block in blocks]
    result, block = [], []
    for line in [*text.splitlines(), ""]:
        if line.lstrip().startswith("|"):
            cells = [plain(c) for c in re.split(r"(?<!\\)\|", line.strip().strip("|"))]
            if not all(re.fullmatch(r":?-+:?", c) for c in cells):
                block.append(cells)
        elif block:
            result.append(block)
            block = []
    return result


def indexed_rows(table, context, variants=VARIANTS):
    indexed = {}
    for cells in table[1:]:
        key = variant_key(cells[0])
        require(key in variants, f"{context}: unrecognized variant row {cells[0]!r}")
        require(key not in indexed, f"{context}: duplicate variant {key}")
        indexed[key] = cells
    require(set(indexed) == set(variants), f"{context}: expected variants {list(variants)}, got {list(indexed)}")
    return indexed


def mean_sd(cell, values, digits, context):
    match = re.fullmatch(r"(\d+(?:\.\d+)?)\s*(?:±|\+/-)\s*(\d+(?:\.\d+)?)", plain(cell))
    require(match is not None, f"{context}: malformed mean/SD {cell!r}")
    expected = (round(statistics.mean(values), digits), round(statistics.stdev(values), digits))
    actual = tuple(map(float, match.groups()))
    require(actual == expected, f"{context}: got {actual}, raw CSV requires {expected} (mean, sample SD)")


def raw_groups(root):
    with (root / RESULTS / "ablation_raw.csv").open(encoding="utf-8", newline="") as source:
        rows = list(csv.DictReader(source))
    require(len(rows) == 1500, f"ablation_raw.csv: expected 1500 rows, got {len(rows)}")
    groups = {v: [] for v in VARIANTS}
    keys = set()
    for row in rows:
        variant = variant_key(row["variant"])
        require(variant in groups, f"ablation_raw.csv: unknown variant {row['variant']!r}")
        require(row["scenario"] in SCENARIOS, f"ablation_raw.csv: unknown scenario {row['scenario']!r}")
        key = (variant, row["scenario"], int(row["seed"]))
        require(0 <= key[2] < 50 and key not in keys, f"ablation_raw.csv: invalid or duplicate trial {key}")
        keys.add(key)
        require(row["censored"] in ("0", "1"), f"ablation_raw.csv: invalid censored flag at {key}")
        for metric, _ in METRICS:
            require(math.isfinite(float(row[metric])) and float(row[metric]) >= 0,
                    f"ablation_raw.csv: invalid {metric} at {key}")
        groups[variant].append(row)
    for variant, group in groups.items():
        require(len(group) == 250, f"ablation_raw.csv: {variant} must have 250 trials")
    return groups


def check_result_tables(root, groups):
    for path in TABLE_SOURCES:
        source_tables = tables((root / path).read_text(encoding="utf-8"), path.suffix == ".tex")
        aggregate = [table for table in source_tables if any(
            len(row) == 6 and re.fullmatch(r"\d+\s*/\s*\d+", row[-1]) for row in table[1:])]
        require(len(aggregate) == 1, f"{path}: expected one six-column aggregate table, got {len(aggregate)}")
        for variant, cells in indexed_rows(aggregate[0], str(path)).items():
            require(len(cells) == 6, f"{path}: {variant} aggregate requires six columns")
            group = groups[variant]
            for i, (metric, digits) in enumerate(METRICS, 1):
                mean_sd(cells[i], [float(row[metric]) for row in group], digits,
                        f"{path}: {variant}/{metric}")
            expected = f"{sum(int(row['censored']) for row in group)}/{len(group)}"
            require(re.sub(r"\s", "", cells[-1]) == expected,
                    f"{path}: {variant}/censored got {cells[-1]}, expected {expected}")

        scenario_tables = [table for table in source_tables
                           if set(SCENARIOS).issubset(set(table[0]))]
        require(len(scenario_tables) == (2 if path == TABLE_SOURCES[0] else 1),
                f"{path}: missing/extra published scenario tables")
        kinds = []
        for table in scenario_tables:
            require(len(table[0]) == 6, f"{path}: scenario header requires six columns")
            indexed = indexed_rows(table, str(path))
            is_switches = bool(re.search(r"±|\+/-", indexed["full"][1]))
            kinds.append("switches" if is_switches else "observer")
            for variant, cells in indexed.items():
                require(len(cells) == 6, f"{path}: {variant} scenario row requires six columns")
                for scenario in SCENARIOS:
                    cell = cells[table[0].index(scenario)]
                    group = [row for row in groups[variant] if row["scenario"] == scenario]
                    context = f"{path}: {variant}/{scenario}"
                    if is_switches:
                        mean_sd(cell, [float(row["switches"]) for row in group], 2, context)
                    else:
                        match = re.fullmatch(r"(\d+(?:\.\d+)?)\s*\((\d+)\)", cell)
                        require(match is not None, f"{context}: malformed observer cell {cell!r}")
                        expected = (round(statistics.mean(float(row["t_legible_s"]) for row in group), 2),
                                    sum(int(row["censored"]) for row in group))
                        require((float(match[1]), int(match[2])) == expected,
                                f"{context}: observer cell {cell!r}, expected {expected}")
        expected_kinds = ["observer", "switches"] if path == TABLE_SOURCES[0] else ["switches"]
        require(sorted(kinds) == expected_kinds, f"{path}: scenario table kinds {kinds}, expected {expected_kinds}")


def check_results_summary(root, groups):
    path = RESULTS / "README.md"
    summaries = [table for table in tables((root / path).read_text(encoding="utf-8"))
                 if len(table[0]) == 2 and any(re.fullmatch(r"\d+/\d+", row[-1])
                                              for row in table[1:])]
    require(len(summaries) == 1, f"{path}: require one labelled two-column censor-count summary")
    table = summaries[0]
    keys = tuple(variant_key(row[0]) for row in table[1:])
    require({"accumulator", "correspondence"}.issubset(keys),
            f"{path}: censor summary must include accumulator and class-correspondence ablations")
    for variant, cells in indexed_rows(table, str(path), keys).items():
        require(variant in groups, f"{path}: unknown summary variant {variant!r}")
        group = groups[variant]
        expected = f"{sum(int(row['censored']) for row in group)}/{len(group)}"
        require(cells[1] == expected, f"{path}: {variant} censor summary {cells[1]}, expected {expected}")


def check_document_roles(root):
    entry = (root / "paper/arxiv/paper_en.md").read_text(encoding="utf-8")
    require(re.search(r"\bcanonical\b", entry, re.I), "paper_en.md: must identify the canonical manuscript")
    destinations = set(markdown_destinations(entry))
    require({"main.tex", "main.pdf"}.issubset(destinations), "paper_en.md: must link main.tex and main.pdf")
    require(not re.search(r"^##\s+(?:Abstract\b|\d+\.\s+(?:Introduction|Method|Experiments))", entry, re.M | re.I),
            "paper_en.md: duplicate manuscript body; this is a canonical-source entry page")
    korean = (root / PAPERS[1]).read_text(encoding="utf-8").split("##", 1)[0]
    require(re.search(r"non[- ]canonical|비정본|보조\s*번역", korean, re.I),
            "paper/paper_draft.md: opening must mark its non-canonical role")
    require("arxiv/main.tex" in set(markdown_destinations(korean)),
            "paper/paper_draft.md: opening must link the canonical arxiv/main.tex")
    for path in (*TABLE_SOURCES, RESULTS / "README.md", Path("paper/arxiv/paper_en.md")):
        text = (root / path).read_text(encoding="utf-8")
        require(not re.search(r"(?<!\d)48\s*/\s*250(?!\d)", text),
                f"{path}: retired 48/250 censor-count claim")


def check_readme_scope(root):
    texts = {path: (root / path).read_text(encoding="utf-8") for path in READMES}
    for path in (READMES[0], READMES[3]):
        require(re.search(r"offline|오프라인", texts[path], re.I), f"{path}: offline measurement scope missing")
    require(re.search(r"research draft|not.{0,35}(?:deploy|certif)", texts[READMES[0]], re.I | re.S),
            "README.md: research-draft/deployment limitation missing")
    require(not re.search(r"아직\s*비어\s*있|^#.*구현\s*\(예정\)", texts[READMES[1]], re.M),
            "src/README.md: obsolete empty/planned-only implementation description")
    require(re.search(r"historical|과거|이전|당시", texts[READMES[2]], re.I),
            "sim/README.md: historical smoke-result scope missing")
    for paragraph in re.split(r"\n\s*\n", texts[READMES[0]]):
        if re.search(r"R5\s*[—–-]|rho.sweep", paragraph, re.I):
            require(not re.search(r"freezing (?:threshold|onset)", paragraph, re.I)
                    or re.search(r"not|offline", paragraph, re.I),
                    "README.md: R5 must not present a physical freezing threshold as measured")


def check_package_versions(root):
    packages = sorted((root / "src").glob("*/package.xml")) + [root / "sim/package.xml"]
    require(len(packages) == 5, f"package version check: expected five package manifests, got {len(packages)}")
    versions = {str(path.relative_to(root)): ET.parse(path).getroot().findtext("version") for path in packages}
    require(all(value and re.fullmatch(r"\d+\.\d+\.\d+", value) for value in versions.values()),
            f"malformed package version: {versions}")
    require(len(set(versions.values())) == 1, f"package versions disagree: {versions}")
    version = next(iter(versions.values()))
    source = (root / "src/compass_core/src/version.cpp").read_text(encoding="utf-8")
    reported = re.findall(r'version\s*\(\s*\)\s*\{\s*return\s+"([^"]+)"', source)
    require(reported == [version], f"version.cpp returns {reported}, package manifests declare {version}")
    smoke = (root / "src/compass_core/test/test_smoke.cpp").read_text(encoding="utf-8")
    expected = re.findall(r'EXPECT_STREQ\(\s*compass::version\(\)\s*,\s*"([^"]+)"\s*\)', smoke)
    require(expected == [version],
            f"test_smoke.cpp expects {expected}, package manifests declare {version}")


def markdown_destinations(text):
    """File links, image targets, reference definitions and HTML href/src.

    Fenced code is ignored. This deliberately is not a full CommonMark parser;
    the supported simple relative paths are the repository's link convention.
    """
    text = re.sub(r"(?ms)^\s*(```|~~~).*?^\s*\1\s*$", "", text)
    for match in re.finditer(r"\]\(\s*(?:<([^>]+)>|([^\s)]+))(?:\s+[\"'][^\n]*?[\"'])?\s*\)", text):
        yield match[1] or match[2]
    for match in re.finditer(r"(?m)^\s{0,3}\[[^\]]+\]:\s*<?([^\s>]+)>?", text):
        yield match[1]
    for match in re.finditer(r"\b(?:href|src)=[\"']([^\"']+)[\"']", text):
        yield match[1]


def check_local_links(root):
    for path in sorted(root.rglob("*.md")):
        relative = path.relative_to(root)
        if any(part.startswith(".") or part in {"build", "install", "log", "node_modules"}
               for part in relative.parts):
            continue
        text = path.read_text(encoding="utf-8")
        # Revision-history notes may intentionally cite paths in an old commit.
        text = re.split(r"(?m)^#{1,3}\s+(?:개정 이력|Revision history|Historical git paths)\b", text, maxsplit=1)[0]
        for destination in markdown_destinations(text):
            url = urlsplit(destination)
            if url.scheme or url.netloc or not url.path or url.path.startswith("/"):
                continue  # remote, anchors, absolute OS/GitHub routes, git:REV:path
            target = unquote(url.path)
            resolved = (path.parent / target).resolve()
            require(resolved.is_relative_to(root.resolve()), f"{relative}: link escapes repository: {destination}")
            require(resolved.exists(), f"{relative}: missing local link target {destination}")


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check_artifact_manifest(root):
    manifest_path = root / "paper/arxiv/artifact_manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    require(manifest.get("schema_version") == 1, "artifact_manifest.json: unsupported schema_version")
    tex = (root / PAPERS[0]).read_text(encoding="utf-8")
    figures = {str(Path("paper/arxiv") / name) for name in
               re.findall(r"\\includegraphics(?:\[[^\]]*\])?\{([^}]+)\}", tex)}
    require(len(figures) == 4, f"main.tex: expected four distinct figure assets, got {sorted(figures)}")
    expected = {str(PAPERS[0]), "paper/arxiv/main.pdf", *figures}
    artifacts = manifest.get("artifacts")
    require(isinstance(artifacts, dict) and set(artifacts) == expected,
            f"artifact_manifest.json: artifacts must be exactly {sorted(expected)}")
    for relative, metadata in artifacts.items():
        path = root / relative
        require(path.resolve().is_relative_to(root.resolve()), f"manifest path escapes repository: {relative}")
        require(isinstance(metadata, dict) and isinstance(metadata.get("sha256"), str)
                and re.fullmatch(r"[0-9a-f]{64}", metadata["sha256"]),
                f"artifact_manifest.json: invalid SHA-256 metadata for {relative}")
        require(sha256(path) == metadata["sha256"], f"artifact_manifest.json: SHA-256 mismatch for {relative}")
    for relative in figures:
        arxiv = root / relative
        canonical = root / "paper/figures" / arxiv.name
        require(arxiv.read_bytes() == canonical.read_bytes(),
                f"figure copies differ: {relative} vs {canonical.relative_to(root)}")
    require((root / "paper/arxiv/main.pdf").read_bytes().startswith(b"%PDF-"), "main.pdf: not a PDF file")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT, help="repository root (default: script parent)")
    args = parser.parse_args(argv)
    root = args.root.resolve()
    errors = []
    try:
        groups = raw_groups(root)
    except (OSError, ValueError, KeyError) as exc:
        errors.append(f"raw data: {exc}")
        groups = None
    checks = [("document roles", lambda: check_document_roles(root)),
              ("active README scope", lambda: check_readme_scope(root)),
              ("package versions", lambda: check_package_versions(root)),
              ("local Markdown links", lambda: check_local_links(root)),
              ("artifact snapshot", lambda: check_artifact_manifest(root))]
    if groups is not None:
        checks[:0] = [("result tables", lambda: check_result_tables(root, groups)),
                      ("results summary", lambda: check_results_summary(root, groups))]
    for name, check in checks:
        try:
            check()
        except (OSError, ValueError, KeyError, IndexError, TypeError, ET.ParseError) as exc:
            errors.append(f"{name}: {exc}")
    for error in errors:
        print(f"RED: {error}", file=sys.stderr)
    if errors:
        return 1
    print("GREEN: repository data, document roles/scope, links, versions and artifact snapshot agree")
    return 0


if __name__ == "__main__":
    sys.exit(main())
