# COMPASS

서비스 로봇의 **사회적 경로 결정(social path decision)** 플래너 연구·구현 저장소입니다. 핵심 아이디어는 사람을 좌·우 어느 쪽으로 지나갈지에 대한 통과 결정의 **시간 일관성(temporal consistency)** 을 1급 설계 목표로 두는 것입니다. 탐욕적 재최적화가 일으키는 진동(우왕좌왕)과 freezing을, 누수 증거 누적(leaky evidence accumulation) 기반 커밋 전환 규칙 + 사전식 안전 오버라이드로 억제합니다.

- **도메인:** 모바일 서비스 로봇(서빙·배송·안내)의 로컬 경로 계획
- **구현 타깃:** ROS2 `Nav2` 커스텀 컨트롤러/플래너 플러그인 (미착수, `src/`)
- **언어/문체 규약:** 문서는 한국어 합쇼체, 기술 용어는 원문 + 백틱 유지

## 현재 상태 (2026-06-23)

| 항목 | 상태 |
| :- | :- |
| 논문 초안 | **v0.5 — 텍스트 완료(text-complete)** |
| 모사 동료 심사 | 4라운드 수렴 (v0.1→v0.5), 편집자 `textComplete=true` |
| 측정 수치 | 전부 `[TBD]` — **날조 없음**(시뮬레이터·실로봇 미구축) |
| 잔여 차단 요인 | 실험(R1~R4·O1) 측정 + 그림 4종 + 구현 |

"텍스트 완료"란 실험 없이 본문 수정만으로 해소 가능한 모든 심사 지적이 반영되었다는 뜻입니다. 게재를 가르는 **경험적 증거(R1~R4)** 는 시뮬레이터/로봇이 갖춰진 뒤의 작업입니다.

## 저장소 구조

```
compass/
├─ README.md              # 이 문서
├─ CLAUDE.md              # Claude Code 세션용 프로젝트 컨텍스트(진실 원천 포인터)
├─ paper/
│  ├─ paper_draft.md      # 현재 논문 (v0.5)
│  ├─ peer_review.md      # 최초 동료 심사 (v0.1 대상)
│  ├─ HANDOFF.md          # 확정 설계(§4) + 개정 로드맵 + 재개 가이드
│  ├─ versions/           # 스냅숏: paper_v0.1 / v0.4 / v0.5
│  ├─ reviews/            # review_history.md (라운드별 심사 전문)
│  └─ figures/            # (예정) 그림 4종 재생성 위치
└─ src/                   # (예정) Nav2 플러그인 구현
```

## 개정 방법론 — 자동 동료 심사 루프

논문 개정은 다음 루프를 수렴할 때까지 반복하여 수행했습니다(단일 워크플로 내부 반복).

1. **개정** — 저자 에이전트가 심사 지시를 모두 반영해 새 버전 산출
2. **심사** — 심사위원 3인(A 이론·정형 / B 시스템·실험 / C 사회·HRI)이 병렬 독립 심사
3. **종합** — 편집자가 결정 + 다음 라운드 지시 도출 → 다시 1로

종료 조건: 편집자가 "텍스트로 해소 가능한 major 항목이 더 없음"(`textComplete`)을 선언하거나 최대 라운드 도달. 모든 라운드에서 측정 수치 날조를 금지하고 각 심사위원이 날조 여부를 능동 점검했습니다.

수렴 경로 요약:

| 버전 | 편집자 결정 | 비고 |
| :- | :- | :- |
| v0.2 | Minor Revision | 실험 불필요 항목(R5·R6·O2·O3·O4) 일괄 반영 |
| v0.3 | Major Revision ↑ | 심사 A가 단일 누적기의 P4 봉쇄 vs O4 헤드룸 **자기모순** 적발 |
| v0.4 | Minor Revision, **textComplete** | 누적기를 정방향 `e^fwd`/역방향 `e^rev`로 분리해 구조적 해소 |
| v0.5 | (정리 패스) | 잔여 13건 사소 명료화 반영 |

라운드별 심사 전문은 [`paper/reviews/review_history.md`](paper/reviews/review_history.md) 참조.

## 다음 단계 (게이팅 순서)

1. **시뮬레이터 구축** — Gazebo/Isaac + 반응형 보행자(PedSim 류). 시뮬–비용함수 순환성(심사 B2) 주의.
2. **구현** — `src/`에 `Nav2` 플러그인 (논문 §4.5 알고리즘 1:1).
3. **실험(R1~R4)** — ablation·비교군·실시간성·time-to-legible. `paper_draft.md`의 `[TBD]`를 측정값으로 교체.
4. **그림 4종** — decision_flow / oscillation_compare / social_cost_field / class_lifecycle.
5. **실로봇 + AR 데모** — BungP 플랫폼, FSD식 AR 궤적 오버레이.

> **COMPASS** = *COMmitment-based PASSing for Social navigation*. 코드·저장소는 COMPASS로 통일했고, 논문 메서드명 통일(현 제목 Commit-and-Cross)은 후속 개정(v0.6)에서 반영 예정입니다.
