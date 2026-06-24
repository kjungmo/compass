# COMPASS

**COMPASS** = *COMmitment-based PASSing for Social navigation*.

서비스 로봇의 **사회적 경로 결정(social path decision)** 플래너 연구·구현 저장소입니다. 핵심 아이디어는 사람을 좌·우 어느 쪽으로 지나갈지에 대한 통과 결정의 **시간 일관성(temporal consistency)** 을 1급 설계 목표로 두는 것입니다. 탐욕적 재최적화가 일으키는 진동(우왕좌왕)과 freezing을, 누수 증거 누적(leaky evidence accumulation) 기반 커밋 전환 규칙 + 사전식 안전 오버라이드로 억제합니다.

- **도메인:** 모바일 서비스 로봇(서빙·배송·안내)의 로컬 경로 계획
- **구현 타깃:** ROS2 **Jazzy** + `Nav2` 커스텀 컨트롤러 플러그인 + Gazebo **Harmonic** 테스트베드
- **언어/문체 규약:** 문서는 한국어 합쇼체, 기술 용어는 원문 + 백틱 유지

## 현재 상태 (2026-06-24)

| 항목 | 상태 |
| :- | :- |
| 논문 초안 | **v0.6** — COMPASS 리브랜딩 + 결정 계층 실측 반영 |
| 모사 동료 심사 | 4라운드 수렴 (v0.1→v0.5, 편집자 `textComplete=true`) |
| 구현 | **완료** — `compass_core`(결정 코어) + `compass_nav2`(Nav2 플러그인) + `compass_msgs` + `compass_sim`(Gazebo) |
| 실험 | **R1·R3·R4 프록시 오프라인 실측 완료** (`compass_eval`); 물리 성과 지표·외부 비교군(R2)·사용자 실험은 `[TBD]` |
| 측정 무결성 | **날조 없음** — 실측값은 `compass_eval`로 재현, 물리 성과 지표는 `[TBD]` 유지 |

## 저장소 구조

```
compass/
├─ README.md / CLAUDE.md
├─ paper/
│  ├─ paper_draft.md      # 현재 논문 (v0.6)
│  ├─ HANDOFF.md          # 확정 설계(§4) + 재개 가이드
│  ├─ versions/           # 스냅숏: v0.1 / v0.4 / v0.5 / v0.6
│  └─ reviews/            # review_history.md
└─ src/
   ├─ compass_core/       # ROS2 비의존 결정 코어 (P1–P5 속성 테스트)
   ├─ compass_nav2/       # nav2_core::Controller 플러그인 + 경로추종 cruise
   ├─ compass_msgs/       # People/Person (/people 입력)
   ├─ compass_eval/       # 오프라인 실험 하니스 (R1/R3/R4 실측, results/)
   └─ ../sim/             # compass_sim — Gazebo Harmonic 테스트베드
```

## 빌드 · 테스트 · 실험

```bash
export MAMBA_ROOT_PREFIX=$HOME/micromamba
micromamba run -n ros2 bash -lc 'set +u; cd ~/kangj/compass && \
  colcon build && source install/setup.bash && colcon test && colcon test-result --verbose'

# 실험 재현 (R1 ablation · R3 latency · R5 rho-sweep)
micromamba run -n ros2 bash -lc 'set +u; cd ~/kangj/compass && source install/setup.bash && \
  ./build/compass_eval/compass_eval ablation src/compass_eval/results && \
  ./build/compass_eval/compass_eval latency && \
  ./build/compass_eval/compass_eval rho_sweep'
```

## 실측 결과 요지 (`src/compass_eval/results/`)

- **R1 진동 제어:** 누수 누적기/class 대응 제거 시 모호 동률에서 조우당 ~98회 진동 대 제안 ≤1회.
- **R3 실시간성:** 주기 worst-case 지연 K=3(8 class) p99 64 µs — 20 Hz 예산의 0.6 % 미만.
- **R5 freezing:** progress-hardening의 ρ-구동률 민감도, freezing 임계 v_lat∈(0.20,0.35) m/s.
- **가설 정정:** −히스테리시스·−경화 단독 제거는 본 영역에서 무퇴행 → 1차 진동 억제 기제는 누적기 자체.

물리 시뮬 의존 성과 지표(성공률·충돌·최소 사회 거리·횡 저크)·외부 비교군(R2)·사용자 실험(R4)은 `[TBD]`.

## 개정 방법론 — 자동 동료 심사 루프

논문 v0.1→v0.5는 (개정 → 심사위원 3인 병렬 독립 심사 → 편집자 종합·차기 지시) 루프를 `textComplete` 선언까지 반복해 수렴했습니다. v0.3에서 단일 누적기의 P4 봉쇄 vs O4 헤드룸 자기모순을 적발해 정방향 `e^fwd`/역방향 `e^rev` 분리로 해소했습니다. 라운드별 심사 전문은 [`paper/reviews/review_history.md`](paper/reviews/review_history.md) 참조. v0.6은 COMPASS 리브랜딩 + `compass_eval` 오프라인 실측 반영입니다.

## 다음 단계

1. **물리 성과 측정** — Gazebo 반응형 보행자 다수 시드 배터리로 성공률·충돌·사회 거리·횡 저크(`[TBD]` 채우기). 시뮬–비용함수 순환성(O1) 통제.
2. **외부 비교군(R2)** — ORCA 등 독립 튜닝 비교.
3. **횡속도 분리** — 코어의 `v_lat=|v_cmd|` 근사를 실 횡속도로 보정(R5 freezing 완화).
4. **실로봇 + AR 데모** — BungP 플랫폼, FSD식 AR 궤적 오버레이.
