#!/usr/bin/env python3
"""check_paper_numbers.py — 논문 수치와 커밋된 측정 산출물의 일치 검사기.

src/compass_eval/results/*.md (권위 원천)에서 수치를 파싱해,
paper/paper_draft.md 가 그 수치(원문 또는 관례적 반올림 표기)를 인용하는지,
그리고 퇴역한 잘못된 수치(64.1 µs, 304 µs, 188 µs, 144 µs, '0.6 %' 예산 주장)가
더 이상 등장하지 않는지 확인합니다.

종료 코드: 0 = 전부 통과(green), 1 = 불일치 존재(red).
표준 라이브러리만 사용합니다.
"""

import re
import sys
from decimal import Decimal, ROUND_HALF_UP
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PAPER = ROOT / "paper" / "paper_draft.md"
RESULTS = ROOT / "src" / "compass_eval" / "results"
R3 = RESULTS / "R3_latency.md"
R1 = RESULTS / "R1_ablation.md"
R5 = RESULTS / "R5_rho_sweep.md"


def rounded_candidates(raw: str):
    """수치 문자열 하나에 대해 논문이 쓸 법한 표기 후보(원문/1자리/2자리 반올림)를 생성."""
    cands = {raw}
    try:
        d = Decimal(raw)
    except ArithmeticError:
        return cands
    for places in ("0.1", "0.01"):
        q = d.quantize(Decimal(places), rounding=ROUND_HALF_UP)
        s = str(q.normalize())
        cands.add(s)
        cands.add(str(q))
    return cands


def table_rows(md_text: str):
    """마크다운 표의 데이터 행(구분선 제외)을 셀 리스트로 반환."""
    rows = []
    for line in md_text.splitlines():
        line = line.strip()
        if not line.startswith("|"):
            continue
        cells = [c.strip() for c in line.strip("|").split("|")]
        if all(re.fullmatch(r":?-+:?", c) for c in cells if c):
            continue  # 구분선
        rows.append(cells)
    return rows


def main() -> int:
    missing = []   # (원천, 라벨, 후보 표기들)
    forbidden_hits = []  # (패턴 설명, 줄번호, 줄 내용)

    for p in (PAPER, R3, R1, R5):
        if not p.exists():
            print(f"[RED] 필수 파일 없음: {p}")
            return 1

    paper = PAPER.read_text(encoding="utf-8")

    def require(source: str, label: str, raw_value: str):
        cands = rounded_candidates(raw_value)
        if not any(c in paper for c in cands):
            missing.append((source, label, sorted(cands)))

    def require_literal(source: str, label: str, literal: str):
        if literal not in paper:
            missing.append((source, label, [literal]))

    # ---------- R3: per-cycle worst-case latency ----------
    r3_rows = table_rows(R3.read_text(encoding="utf-8"))
    r3_data = {}
    for cells in r3_rows:
        if len(cells) >= 7 and re.fullmatch(r"[123]", cells[0]):
            k = int(cells[0])
            r3_data[k] = dict(
                mean=cells[2], p50=cells[3], p99=cells[4],
                max=cells[5], occ=cells[6].rstrip("%"),
            )
    if sorted(r3_data) != [1, 2, 3]:
        print(f"[RED] R3 표 파싱 실패: {sorted(r3_data)} — {R3}")
        return 1
    for k, row in sorted(r3_data.items()):
        require("R3", f"K={k} p99 (µs)", row["p99"])
        require("R3", f"K={k} max (µs)", row["max"])
    # 20 Hz 예산 점유(최악 K=3)는 본문 주장과 직결되므로 명시 인용을 요구
    require("R3", "K=3 20Hz 예산 점유(%)", r3_data[3]["occ"])

    # ---------- R1: ablation aggregate + per-scenario switches ----------
    r1_text = R1.read_text(encoding="utf-8")
    r1_rows = table_rows(r1_text)
    pm = re.compile(r"([\d.]+)\s*\+/-\s*([\d.]+)")
    cens = re.compile(r"(\d+)/250")
    agg_seen = scen_seen = 0
    for cells in r1_rows:
        variant = cells[0] if cells else ""
        if not variant or variant in ("변형",):
            continue
        joined = " | ".join(cells[1:])
        for m in pm.finditer(joined):
            # 종합표(±와 /250 동반)와 시나리오별 전환수 표의 모든 mean/sd 값
            require("R1", f"{variant}: {m.group(0)} (mean)", m.group(1))
            require("R1", f"{variant}: {m.group(0)} (sd)", m.group(2))
        for m in cens.finditer(joined):
            require_literal("R1", f"{variant}: 검열율 {m.group(0)}", m.group(0))
        if pm.search(joined):
            if cens.search(joined):
                agg_seen += 1
            else:
                scen_seen += 1
    if agg_seen < 6:
        print(f"[RED] R1 종합표 파싱 실패(행 {agg_seen}/6) — {R1}")
        return 1

    # 본문이 서술로 인용하는 핵심 검열 사실(간헐=제안 50/50, 단순드웰 0/50 등)
    require_literal("R1", "검열 사실 '50/50' (intermittent 제안 전면 검열 등)", "50/50")
    require_literal("R1", "검열 사실 '0/50' (비검열 사례)", "0/50")
    # 시나리오별 t-legible에서 본문이 인용하는 대표값
    require("R1", "clean_commit 제안 t-legible (2.37 s)", "2.37")
    require("R1", "near_tie argmin t-legible (7.89 s)", "7.89")

    # ---------- R5: v_lat freezing threshold ----------
    r5_text = R5.read_text(encoding="utf-8")
    r5_rows = table_rows(r5_text)
    commit_by_vlat = {}
    for cells in r5_rows:
        if len(cells) >= 6 and re.fullmatch(r"0\.\d+", cells[0]):
            commit_by_vlat[cells[0]] = cells[2]
    if not commit_by_vlat:
        print(f"[RED] R5 표 파싱 실패 — {R5}")
        return 1
    committing = [v for v, c in commit_by_vlat.items() if c.startswith("50/")]
    blocked = [v for v, c in commit_by_vlat.items() if c.startswith("0/")]
    if committing and blocked:
        lo, hi = max(committing), min(blocked)  # 문자열 비교로 충분(0.xx 고정 폭)
        require_literal("R5", f"freezing 임계 하한 v_lat={lo}", lo)
        require_literal("R5", f"freezing 임계 상한 v_lat={hi}", hi)

    # ---------- 퇴역 수치 금지 목록 ----------
    forbidden = [
        (r"64\.1", "퇴역: K=3 p99 64.1 µs (실측 58.642)"),
        (r"(?<![\d.])304(?![\d.%])", "퇴역: K=3 max 304 µs (실측 651.257)"),
        (r"(?<![\d.])188(?![\d.])", "퇴역: K=1 max 188 µs (실측 153.607)"),
        (r"(?<![\d.])144(?![\d.])", "퇴역: K=2 max 144 µs (실측 356.086)"),
        (r"0\.6\s*%", "퇴역: '0.6 % 미만' 예산 주장 (실측 K=3 점유 1.3025 %)"),
        (r"p99\s*64(?![\d.])", "퇴역: 'p99 64 µs' 표기"),
        (r"3\.1/7\.1/64\.1", "퇴역: p99 3.1/7.1/64.1 µs 묶음 표기"),
    ]
    for lineno, line in enumerate(paper.splitlines(), start=1):
        for pat, desc in forbidden:
            if re.search(pat, line):
                forbidden_hits.append((desc, lineno, line.strip()[:120]))

    # ---------- 보고 ----------
    ok = not missing and not forbidden_hits
    if missing:
        print(f"[RED] 논문에 없는 권위 수치 {len(missing)}건:")
        for source, label, cands in missing:
            print(f"  - [{source}] {label}: 다음 표기 중 어느 것도 논문에 없음 -> {cands}")
    if forbidden_hits:
        print(f"[RED] 퇴역 수치 발견 {len(forbidden_hits)}건:")
        for desc, lineno, line in forbidden_hits:
            print(f"  - {desc} @ paper/paper_draft.md:{lineno}: {line}")
    if ok:
        print("[GREEN] check_paper_numbers: 논문 수치가 커밋된 산출물과 일치하고, "
              "퇴역 수치(64.1/304/188/144/'0.6 %')는 등장하지 않습니다.")
        print(f"  검사 원천: {R3.name}, {R1.name}, {R5.name} "
              f"(R1 종합 {agg_seen}행 · 시나리오 {scen_seen}행)")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
