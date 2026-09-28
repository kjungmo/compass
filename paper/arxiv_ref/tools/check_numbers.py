#!/usr/bin/env python3
"""Number/claim guard for the re-typeset manuscript in paper/arxiv_ref.

Three checks, standard library only (exit 0 = GREEN, 1 = RED):
  1. Every published R1 table cell in the LaTeX package (aggregate, per-scenario
     switches, per-scenario observer) equals the value recomputed from
     src/compass_eval/results/ablation_raw.csv (reuses scripts/check_repo_consistency).
  2. The repository's existing numeric guard scripts/check_paper_numbers.py is run
     unchanged against the flattened LaTeX text instead of paper/paper_draft.md.
  3. Every row of paper/arxiv_ref/NUMBERS.md is verified: the printed TeX literal
     occurs in the manuscript, and its check (artifact literal, CSV statistic,
     YAML knob, or closed-form derivation) reproduces the printed value.
Usage: python3 paper/arxiv_ref/tools/check_numbers.py
"""
import csv
import importlib
import math
import re
import statistics
import sys
import tempfile
from pathlib import Path

PKG = Path(__file__).resolve().parents[1]
ROOT = PKG.parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
crc = importlib.import_module("check_repo_consistency")
RES = ROOT / "src" / "compass_eval" / "results"


def flatten(path):
    text = path.read_text(encoding="utf-8")
    def sub(m):
        name = m.group(1)
        p = PKG / (name if name.endswith(".tex") else name + ".tex")
        return flatten(p)
    return re.sub(r"\\input\{([^}]+)\}", sub, text)


def tex_text():
    t = flatten(PKG / "main.tex")
    return re.sub(r"(?m)(?<!\\)%.*$", "", t)


def check_tables(tex, groups):
    t = re.sub(r"\\stdv\{([^{}]*)\}", r"$\\pm$\1", tex)
    src = crc.tables(t, latex=True)
    agg = [tb for tb in src if any(len(r) == 6 and re.fullmatch(r"\d+\s*/\s*\d+", r[-1]) for r in tb[1:])]
    crc.require(len(agg) == 1, f"expected one aggregate table, got {len(agg)}")
    n = 0
    for variant, cells in crc.indexed_rows(agg[0], "tab:main").items():
        g = groups[variant]
        for i, (metric, digits) in enumerate(crc.METRICS, 1):
            crc.mean_sd(cells[i], [float(r[metric]) for r in g], digits, f"tab:main {variant}/{metric}")
            n += 1
        exp = f"{sum(int(r['censored']) for r in g)}/{len(g)}"
        crc.require(re.sub(r"\s", "", cells[-1]) == exp, f"tab:main {variant} censored {cells[-1]} != {exp}")
        n += 1
    kinds = []
    for tb in [tb for tb in src if set(crc.SCENARIOS).issubset(set(tb[0]))]:
        idx = crc.indexed_rows(tb, "scenario table")
        is_sw = bool(re.search(r"±|\+/-", idx["full"][1]))
        kinds.append("switches" if is_sw else "observer")
        for variant, cells in idx.items():
            for s in crc.SCENARIOS:
                cell = cells[tb[0].index(s)]
                g = [r for r in groups[variant] if r["scenario"] == s]
                if is_sw:
                    crc.mean_sd(cell, [float(r["switches"]) for r in g], 2, f"{variant}/{s}")
                else:
                    m = re.fullmatch(r"(\d+(?:\.\d+)?)\s*\((\d+)\)", cell)
                    crc.require(m is not None, f"malformed observer cell {cell!r}")
                    exp = (round(statistics.mean(float(r["t_legible_s"]) for r in g), 2),
                           sum(int(r["censored"]) for r in g))
                    crc.require((float(m[1]), int(m[2])) == exp, f"{variant}/{s}: {cell} != {exp}")
                n += 1
    crc.require(sorted(kinds) == ["observer", "switches"], f"scenario table kinds {kinds}")
    return n


def run_legacy_guard(tex):
    mod = importlib.import_module("check_paper_numbers")
    with tempfile.NamedTemporaryFile("w", suffix=".tex", delete=False, encoding="utf-8") as f:
        f.write(tex)
        tmp = Path(f.name)
    mod.PAPER = tmp
    try:
        return mod.main()
    finally:
        tmp.unlink()


def csv_stat(spec, rows):
    variant, scenario, metric, stat = spec.split("/")
    vals = [float(r[metric]) for r in rows
            if crc.variant_key(r["variant"]) == variant and (scenario == "*" or r["scenario"] == scenario)]
    crc.require(vals, f"no CSV rows for {spec}")
    return {"mean": statistics.mean, "max": max, "min": min, "sum": sum, "count": len}[stat](vals)


def yaml_value(key):
    text = (ROOT / "src/compass_nav2/config/compass_params.yaml").read_text(encoding="utf-8")
    m = re.search(rf"(?m)^\s*{re.escape(key)}:\s*([^\s#]+)", text)
    crc.require(m is not None, f"yaml key {key} missing")
    return m.group(1)


def ramp_end_evidence():
    """Noiseless mid_reversal ramp through the leaky recursion at default knobs."""
    T, dt, lam, df, emax = 200, 0.05, 0.97, 0.05, 0.50
    e = 0.0
    for i in range(T):
        d = -0.25 + 0.50 * i / (T - 1)
        e = min(emax, max(0.0, lam * e + (d - df) * dt))
    return e


def ramp_cycles_above(th):
    T = 200
    return sum(1 for i in range(T) if -0.25 + 0.50 * i / (T - 1) > th)


def n_resp(d, ebar, lam=0.97, dt=0.05, df=0.05):
    a = (d - df) * dt
    return math.ceil(math.log(1 - ebar * (1 - lam) / a) / math.log(lam))


ENV = {"math": math, "ceil": math.ceil, "sqrt": math.sqrt, "log": math.log,
       "n_resp": n_resp, "ramp_end_evidence": ramp_end_evidence, "ramp_cycles_above": ramp_cycles_above}


def numbers_rows():
    rows = []
    for line in (PKG / "NUMBERS.md").read_text(encoding="utf-8").splitlines():
        if not line.startswith("| ") or line.startswith("| #") or set(line) <= set("|- "):
            continue
        cells = [c.strip() for c in line.strip().strip("|").split(" | ")]
        crc.require(len(cells) == 6, f"NUMBERS.md malformed row: {line[:80]}")
        rows.append(cells)
    return rows


def unquote(cell):
    m = re.fullmatch(r"`(.*)`", cell)
    crc.require(m is not None, f"expected code span: {cell}")
    return m.group(1).replace("\\|", "|")


def check_numbers_md(tex, raw):
    failures, n = [], 0
    for rid, printed, _where, _artifact, _field, check in numbers_rows():
        n += 1
        p = unquote(printed)
        if p not in tex:
            failures.append(f"{rid}: printed literal not in manuscript: {p!r}")
        kind, _, arg = unquote(check).partition(":")
        try:
            if kind == "table":
                pass  # verified cell-by-cell in check_tables
            elif kind == "lit":
                path, _, needle = arg.partition("::")
                txt = (ROOT / path).read_text(encoding="utf-8")
                if needle not in txt:
                    failures.append(f"{rid}: {needle!r} not found in {path}")
            elif kind == "yaml":
                key, _, val = arg.partition("=")
                if yaml_value(key) != val:
                    failures.append(f"{rid}: yaml {key}={yaml_value(key)} != {val}")
            elif kind in ("csv", "calc"):
                expr, _, want = arg.rpartition("=")
                got = csv_stat(expr, raw) if kind == "csv" else eval(expr, {"__builtins__": {}, **ENV})  # expressions come only from the committed NUMBERS.md
                digits = len(want.split(".")[1]) if "." in want else 0
                if round(float(got), digits) != float(want):
                    failures.append(f"{rid}: {expr} = {got!r}, printed {want}")
            else:
                failures.append(f"{rid}: unknown check kind {kind!r}")
        except (OSError, ValueError, KeyError, SyntaxError, ZeroDivisionError) as exc:
            failures.append(f"{rid}: {exc}")
    return n, failures


def main():
    tex = tex_text()
    status = 0
    groups = crc.raw_groups(ROOT)
    raw = [r for g in groups.values() for r in g]
    try:
        cells = check_tables(tex, groups)
        print(f"[GREEN] R1 tables: {cells} published cells match ablation_raw.csv")
    except (ValueError, KeyError, IndexError) as exc:
        print(f"[RED] R1 tables: {exc}")
        status = 1
    print("--- scripts/check_paper_numbers.py against the flattened LaTeX package ---")
    if run_legacy_guard(tex) != 0:
        status = 1
    n, failures = check_numbers_md(tex, raw)
    if failures:
        status = 1
        print(f"[RED] NUMBERS.md: {len(failures)} of {n} rows failed")
        for f in failures:
            print("  -", f)
    else:
        print(f"[GREEN] NUMBERS.md: all {n} claim rows traced and reproduced")
    return status


if __name__ == "__main__":
    sys.exit(main())
