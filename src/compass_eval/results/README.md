# COMPASS 오프라인 실험 하니스 — 결과

`compass_eval` 은 **실제 결정 코어**(`compass_core::DecisionCore`)를 재현 가능한 조우
시나리오로 구동해 행동 일관성 지표를 **실측**합니다. 이 디렉터리의 R1/R5 수치는
결정 스트림에서 계산되며, `ablation_raw.csv`로 R1 원자료를 공개합니다.
R3는 이전 소스/장비에서 측정한 보관 지연 기록입니다. 물리 성능 결과가 아닙니다.

## 재현

```bash
# 저장소 루트, ROS 2 Jazzy 및 선언된 의존성이 설치/source된 환경
colcon build --base-paths src --packages-up-to compass_eval
source install/setup.bash
mkdir -p /tmp/compass_eval_results
./build/compass_eval/compass_eval ablation /tmp/compass_eval_results
./build/compass_eval/compass_eval latency
./build/compass_eval/compass_eval rho_sweep
```

ablation은 CSV를 지정 디렉터리에 기록하고 표를 stdout에 출력합니다. 나머지
명령은 표를 stdout에 출력합니다. 새 측정으로 보관 결과를 덮어쓰지 마십시오.
ROS 없이 현재 CSV를 재검증하려면 저장소 루트에서
`bash scripts/test_compass_observability.sh`를 실행합니다. 정확한 난수 시퀀스
회귀는 현재 g++/libstdc++ 하니스를 대상으로 하며 다른 표준 라이브러리까지
같음을 보장하지 않습니다. 같은 소스를 LLVM libc++로 빌드하면 1,500행 중 425행이
달라집니다. 제안 방법과 두 노브 ablation의 전환수는 750행 모두 그대로이고, 전환수가
바뀌는 것은 비교군뿐입니다. 기록과 검사 명령은
[`stdlib_sensitivity/`](stdlib_sensitivity/README.md)와
`scripts/check_stdlib_sensitivity.sh`에 있습니다. 새 실행은 소스/설정/입력 해시, 컴파일러·라이브러리·
빌드 플래그, 장비와 명령을 기록해야 합니다. R3 숫자의 일치는 재현 조건이 아닙니다.

## 결과 버전과 해석

원본 보관점은 Git 커밋 `9fe495a`입니다. 관찰자 결과는 세 버전이 있고, 모두 같은
1,500개 결정 스트림을 채점합니다(전환수·부호변화율·엔트로피는 세 CSV에서 동일).

1. `9fe495a`: 확률 공간 재귀. 사후확률이 정확히 1.0으로 반올림되어 이후 반전을
   받아들이지 못하는 수치 결함이 있습니다.
2. `5343d45`(PR #12): log-odds로 포화만 고친 수치 교정본. 마지막 표본에서의 첫
   임계 통과도 도달로 셉니다.
3. 현재(`ablation_raw.csv`): log-odds에 더해, 끝-접미 확신 구간의 첫 표본 뒤로
   최소 0.30초의 실제 관측 후속 구간을 요구하는 **지표 정의 변경**입니다.
   0.30초 값은 PR #12 리뷰(마지막 표본에서만 확신한 22행) 이후에 정한 것이며
   사전 등록된 값이 아닙니다.

현재 보고값은 **끝-접미 시각(endpoint-suffix time) `t_sfx`** 입니다. 논문 §4.7의
고정 유지창 `Δ_hold`를 쓰는 `t_legible`을 구현한 것이 아니며, "가장 엄격한" 또는
보수적인 유지 조건도 아닙니다. CSV 열 이름 `t_legible_s`는 보관 호환을 위해서만
유지합니다. 세 버전의 원자료, 행 단위 비교표와 요약은
[`observer_versions/`](observer_versions/README.md)에 있고
`scripts/check_observer_versions.py`가 보관 CSV와 재생성 열의 일치를 검사합니다.
서로 다른 버전을 같은 관찰자처럼 합쳐서는 안 됩니다.

현재 검열 분모는 각 변형의 5 시나리오 × 50 시드입니다:

| Variant | Censored trials |
|---|---|
| full (제안) | 0/250 |
| -hysteresis | 0/250 |
| -progress hardening | 0/250 |
| -accumulator (argmin) | 72/250 |
| simple-dwell (O4) | 50/250 |
| -class correspondence | 71/250 |

이 observer는 모든 정책에 적용한 공통 **결정-라벨 스트림 점수 규약**이며 독립적인
motion/human legibility 척도가 아닙니다. 검열된 시행에는 표의 계산상 10초 지평값이
들어가므로 표의 평균은 eventual legibility 시간의 추정치가 아닙니다. 최종 라벨을
바꾸지 않는 정책은 필요한 대응에 실패해도 즉시 가독으로 채점될 수 있습니다.

## 측정 대상(무엇을 실측하고 무엇이 [TBD]인가)

오프라인 하니스는 결정 코어가 산출하는 **통과-측 결정 스트림**에서 다음을 산출합니다.

- 조우당 결정 전환 횟수, 횡 부호 변화율(/s), 결정 엔트로피(bits/decision)
- 끝-접미 시각 `t_sfx` — §4.7 관찰자의 사전확률·우도(ε=0.2, p\*=0.9)를 결정 스트림에 적용한
  끝-접미 통계. 0.30초 후속 관측이 없으면 우측 검열합니다. 고정 `Δ_hold`의 `t_legible`이 아닙니다
- (R3) `DecisionCore::step` 의 주기당 worst-case 지연, 명시 열거 폭 `2^K`

물리 시뮬레이션이 있어야만 측정 가능한 항목은 **[TBD]** 로 둡니다(본 하니스 범위 밖):
성공률·충돌률·최소 사회 거리·횡 저크(속도 궤적 필요), 외부 비교군(ORCA 등) R2,
가지치기 후 실제 `|S_k|` 분포(실 costmap 필요), 사용자 실험 R4.

## 방법

- 시나리오 5종 × 시드 50 × 변형 6종, dt=0.05 s(20 Hz), 지평 200 주기(10 s).
- `adv(i)=J_R−J_L` (양수 ⇒ L 우위) 를 스크립트로 생성 + 가우시안 잡음. c\* 는 R 출발.
  - `near_tie` (σ=0.12, 평균 0): 모호한 동률 — 진동 스트레스.
  - `transient_spike`: 기저 R 우위, 80–100주기에 일시 L 우위 스파이크 — 스퍼리어스 억제.
  - `mid_reversal`: 우위가 −0.25→+0.25 로 서서히 역전.
  - `clean_commit` (adv=+0.40): 명확한 L 우위 — 신속 커밋 기대.
  - `intermittent` (8주기 +0.45 / 4주기 −0.05): 간헐 우위 — 누적기 대 드웰(O4) 구분.
- 변형: `full`(제안) · `−hysteresis`(Δ_floor=0) · `−progress hardening`(k_ρ=0) ·
  `−accumulator`(즉시 argmin) · `simple-dwell`(연속 드웰 0.6 s) · `−class correspondence`
  (근접 동률대 무작위 깜빡임을 주입하는 합성 comparator이며 실제 추적 ID 오류
  실험이나 class lifecycle의 충실한 단독 ablation은 아님).
- ρ 구동: ρ 는 횡 기동 진행도이므로 순항 0.5 m/s 의 횡 성분(소각 회피 ≈0.1배)을
  반영해 `v_lat=0.05` 로 둡니다. 코어가 `v_lat=|v_cmd|` 로 단순화한 점을 실험에서 보정.

전환수는 연속 **출력** 라벨의 변경을 셉니다. 초기 내부 라벨 R에서 첫 출력으로의
변경은 세지 않으므로, `clean_commit`의 argmin 전환수 0은 첫 출력에서 곧바로
L을 고른 것을 포함합니다. 시나리오의 Gaussian 잡음은 분포상 유계가 아니므로
모든 가능한 입력이 정형 명제의 정규화된 `D_max` 범위를 만족한다고 가정할 수
없습니다. 이 스크립트형 스트레스 결과와 명제의 조건부 보장은 구분해야 합니다.

## 핵심 결과 요약 (정직한 해석)

**강하게 확인된 것**

1. **진동 제어(핵심 주장).** `−accumulator`(즉시 argmin)·`−class correspondence` 는
   조우당 30–98회 전환으로 진동하는 반면, `full` 은 ≤1회로 커밋합니다(near_tie:
   argmin 98.2±7.9 vs full 0). 이 구분은 위 스크립트형 오프라인 실험에 한정됩니다.
2. **스퍼리어스 억제.** `transient_spike` 에서 full=0 전환(스파이크 무시), argmin=2.
3. **신속 커밋.** `clean_commit` 에서 full 은 1회 전환, `t_sfx` 2.37 s.
4. **결정 스트림 observer.** argmin/합성 −class comparator의 검열은
   각각 72/250, 71/250입니다(near_tie: argmin 7.90 s vs full 0.05 s).
   사람의 판단이나 실제 움직임 가독성을 입증하는 결과가 아닙니다.

**가설과 달랐던 것(정직히 보고)**

- `−hysteresis`(Δ_floor=0)·`−progress hardening`(k_ρ=0) 은 본 시나리오들에서 full 대비
  **퇴행이 관찰되지 않았습니다.** 누수 누적기의 임계 `E0`+누수가 이미 영평균 잡음을
  기각하므로, 본 동작 영역에서 1차 진동 억제 기제는 Δ_floor·k_ρ 가 아니라 **누적기**
  자체입니다. (논문이 사전 등록한 "Δ_floor 제거 → 전환 폭증" 가설은 본 영역에서
  성립하지 않음 — 측정에 의한 가설 정정.)
- `simple-dwell`(O4) 은 `intermittent` 에서 교착(전환 0)하지만, 고정된 초기
  라벨 때문에 결정-스트림 관찰자에는 즉시 가독으로 보입니다. `full`은 50개
  시드에서 1.55--2.10초에 전환하고 평균 `t_sfx` 4.11초, 검열 0/50입니다.
  따라서 이 프록시만으로 responsiveness를 판정할 수 없습니다.

**ρ-율 민감도(R5 직결, 오프라인 전환 봉쇄).** `R5_rho_sweep.md`: clean_commit 에서 v_lat≤0.20 m/s
면 full 이 항상 커밋(50/50, `t_sfx`≈2.4 s)하지만 v_lat≥0.35 m/s 면 **전환이 봉쇄**
(0/50)됩니다. progress hardening 이 ρ 포화로 E_th 를 E0(1+k_ρ) 까지 올려 정당한 전환마저
막는 **오프라인 전환 봉쇄 구간이 v_lat∈(0.20,0.35)** 에 있음을 실측했습니다. 이는
스크립트형 진행 입력과 기존 `v_lat=|v_cmd|` 단순화의 민감도이며, 실제 로봇 정지·물리
freezing·목표 실패를 입증하지 않습니다.

산출물: `R1_ablation.md` · `R3_latency.md` · `R5_rho_sweep.md` · `ablation_raw.csv`(원자료).

Round 5: 관찰자는 log-odds로 수치 포화를 고쳤고, 그와 별개로 0.30초 후속 관측
규칙(사후 선택)을 더했습니다. 지평 마지막 표본에서 처음 임계값을 넘는 경우를 포함해
후속 관측이 0.30초 미만이면 검열합니다. R1 행동 결과는 EPYC 9V74에서 재실행했고,
R3 latency는 기존 측정입니다.

## 별도 연구 산출물

- [`responsive_profile/`](responsive_profile/): 3,750개 paired offline 실험의
  요약과 seed별 스위치/clean-case recall. 진행 입력은 스크립트형이며 독립적인
  물리 oracle 검증이 아닙니다. 검증 transcript는 당시 실행의 역사적 기록입니다.
- [`closed_loop_diagnostic/`](closed_loop_diagnostic/): scripted-cost unicycle
  diagnostic. legacy proxy는 60초 timeout, 두 measured 설정은 모두 17.45초 goal로
  동일합니다. responsiveness 노브의 독립 효과나 실제 Nav2/Gazebo 성능을 보여주지
  않습니다. 원본 motion JSONL은 gate 재실행 시 생성되고 요약에 trace 해시가 있습니다.
- [물리 pilot 계획](../RESPONSIVENESS.md#physical-pilot-480-planned-cases):
  480개 **pending** 사례. 기하·actor trace·독립 oracle·adapter와 provenance가
  결합되기 전 실행하거나 결과 분모에 넣어서는 안 됩니다.
