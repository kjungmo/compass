# COMPASS 오프라인 실험 하니스 — 결과

`compass_eval` 은 **실제 결정 코어**(`compass_core::DecisionCore`)를 재현 가능한 조우
시나리오로 구동해 행동 일관성 지표를 **실측**합니다. 측정값 날조는 없습니다 — 모든
수치는 결정 스트림에서 결정적으로 계산되며, `ablation_raw.csv` 로 원자료를 공개합니다.

## 재현
```bash
export MAMBA_ROOT_PREFIX=$HOME/micromamba
micromamba run -n ros2 bash -lc 'set +u; cd ~/kangj/compass && \
  colcon build --packages-select compass_eval && source install/setup.bash && \
  ./build/compass_eval/compass_eval ablation src/compass_eval/results && \
  ./build/compass_eval/compass_eval latency && \
  ./build/compass_eval/compass_eval rho_sweep'
```

## 측정 대상(무엇을 실측하고 무엇이 [TBD]인가)

오프라인 하니스는 결정 코어가 산출하는 **통과-측 결정 스트림**에서 다음을 산출합니다.
- 조우당 결정 전환 횟수, 횡 부호 변화율(/s), 결정 엔트로피(bits/decision)
- time-to-legible — 논문 §4.7 베이즈 관찰자(ε=0.2, p\*=0.9, 우측검열)를 결정 스트림에 적용
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
  (정체성 상실 — 근접 동률대 무작위 깜빡임).
- ρ 구동: ρ 는 횡 기동 진행도이므로 순항 0.5 m/s 의 횡 성분(소각 회피 ≈0.1배)을
  반영해 `v_lat=0.05` 로 둡니다. 코어가 `v_lat=|v_cmd|` 로 단순화한 점을 실험에서 보정.

## 핵심 결과 요약 (정직한 해석)

**강하게 확인된 것**
1. **진동 제어(핵심 주장).** `−accumulator`(즉시 argmin)·`−class correspondence` 는
   조우당 30–98회 전환으로 진동하는 반면, `full` 은 ≤1회로 커밋합니다(near_tie:
   argmin 98.2±7.9 vs full 0). 시드 분산이 작아 견고합니다.
2. **스퍼리어스 억제.** `transient_spike` 에서 full=0 전환(스파이크 무시), argmin=2.
3. **신속 커밋.** `clean_commit` 에서 full 은 1회 전환, t-legible 2.37 s.
4. **가독성.** 진동 변형은 사후확률이 안정되지 못해 자주 비가독(argmin/−class 검열
   73–74/250; near_tie t-legible 7.9 s vs full 0.05 s).

**가설과 달랐던 것(정직히 보고)**
- `−hysteresis`(Δ_floor=0)·`−progress hardening`(k_ρ=0) 은 본 시나리오들에서 full 대비
  **퇴행이 관찰되지 않았습니다.** 누수 누적기의 임계 `E0`+누수가 이미 영평균 잡음을
  기각하므로, 본 동작 영역에서 1차 진동 억제 기제는 Δ_floor·k_ρ 가 아니라 **누적기**
  자체입니다. (논문이 사전 등록한 "Δ_floor 제거 → 전환 폭증" 가설은 본 영역에서
  성립하지 않음 — 측정에 의한 가설 정정.)
- `simple-dwell`(O4) 은 `intermittent` 에서 교착(전환 0, 비가독)하나, full 도 이
  영역에서 늦게(거의 지평 끝) 커밋해 함께 비가독에 가깝습니다 — 기본 노브에서
  간헐 우위 커밋이 느림(파라미터 민감도).

**ρ-율 민감도(R5 직결, freezing).** `R5_rho_sweep.md`: clean_commit 에서 v_lat≤0.20 m/s
면 full 이 항상 커밋(50/50, t-legible≈2.4 s)하지만 v_lat≥0.35 m/s 면 **전환이 봉쇄**
(0/50)됩니다. progress hardening 이 ρ 포화로 E_th 를 E0(1+k_ρ) 까지 올려 정당한 전환마저
막는 **freezing 임계가 v_lat∈(0.20,0.35)** 에 있음을 실측. 기본 노브의 비봉쇄 구간이
좁다는 R5 민감도 발견이며, 코어의 `v_lat=|v_cmd|` 단순화를 향후 횡속도 분리로 보정할
근거입니다.

산출물: `R1_ablation.md` · `R3_latency.md` · `R5_rho_sweep.md` · `ablation_raw.csv`(원자료).
