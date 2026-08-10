# 인용·서지 감사 보고서 (citation-audit)

**일자**: 2026-08-10
**대상**: `paper/arxiv/main.tex` (별도 `.bib` 없이 본문 내 `thebibliography` 블록)
**서지 항목**: 28 · **인용 호출**: 35회 · **심사 대상(인용된 고유 키)**: 27
**심사자**: `gpt-5.6-sol`, 추론 `xhigh`, 읽기전용 샌드박스, 웹검색 필수
**스레드 정책**: 5개 독립 배치 스레드 (이전 심사 맥락 미주입)
**최상위 판정**: **FAIL** (`wrong_context`) — 문맥 오용이 존재하므로 제출 전 교정 필요

## 요약

| 판정 | 건수 | 의미 |
|---|---|---|
| KEEP | 7 | 서지·문맥 모두 정상 |
| FIX | 12 | 서지 오류. 인용 위치는 타당 |
| REPLACE | 8 | 실재하는 문헌이나, 그 문헌이 뒷받침하지 않는 주장에 붙어 있음 |
| REMOVE | 0 | 날조된 문헌 없음 |

**날조 인용은 한 건도 없습니다.** 문제는 전부 (a) 제목에 없는 설명구를 제목 안에 넣은 서지 오류, (b) 실재 문헌을 그 문헌이 말하지 않는 주장의 근거로 쓴 문맥 오용입니다. 후자가 심사에서 더 위험합니다.

### 독립 검증 표본

심사자 결과를 그대로 채택하지 않고 4건을 별도 웹조회로 교차 확인했으며, **4건 모두 일치**했습니다.

| 항목 | 심사자 지적 | 독립 확인 |
|---|---|---|
| `mavrogiannis2022momentum` | `J. Thomason` → `W. Thomason` | 일치 (Wil Thomason) |
| `francis2023principles` | 정본은 ACM THRI 14(2) Art.34, 2025 | 일치 |
| `cao2019dynamicchannel` | `Z. Cao·K. Iba` → `C. Cao·S. Iba` | 일치 (Chao Cao, Soshi Iba) |
| `degroot2024topology` | 주기 간 위상 일관성을 이미 형식화함 | 일치 (T-RO 원문이 topology 정보를 반복 주기에 전파해 일관성 유지) |

## 최우선 조치 — 신규성 주장에 걸리는 건 (1건)

### `degroot2024topology` — 본문 §관련연구 `main.tex:78`

판정 자체는 FIX지만, 파급은 서지를 넘어섭니다. 본문은 이 문헌이 "commitment 자체를 형식화하지는 않는다"고 선을 긋는데, 심사자는 **T-RO 원문이 일관성 파라미터 `c_i`를 명시적으로 정의하고 `c_i = 0`이면 직전 homotopy class가 유효한 한 그것을 강제한다**고 지적합니다. 저희 독립 조회로도 "topology 정보를 연속 주기에 전파해 식별을 일관되게 유지"가 확인됩니다.

- **의미**: "주기 간 위상 일관성"이라는 층위 자체는 이미 선점돼 있습니다. COMPASS가 방어 가능한 차별점은 그보다 좁습니다 — **track ID에 묶인 class 생애주기(spawn·removal·merge·split)** 와 **전환율 상한(P1·P2)** 입니다.
- **조치**: §관련연구의 대비 문장을 이 좁은 축으로 재서술하고, 서지의 연도를 2025(vol. 41, pp. 110–126), 저자를 `D. M. Gavrila`로 교정.
- **이건 kill-argument가 아니라 서지 감사에서 먼저 나왔습니다.** 신규성 문장은 제출 전 반드시 손봐야 합니다.

## REPLACE (8건) — 문맥 오용

각 항목은 "문헌을 빼라"가 아니라 **"그 문헌이 실제로 말하는 것에만 붙여라"** 입니다.

### `fox1997dwa` — `main.tex:62`
DWA의 속도 명령 선택에는 정본이지만, 본문이 붙인 **"memoryless라서 진동에 취약하다"** 는 인과 주장은 이 논문이 세우지 않습니다. → DWA 서술용으로 유지하고, 진동 취약성 주장에는 별도 근거를 붙이거나 주장을 완화.

### `rosmann2017teb` — `main.tex:62`, `main.tex:78`
제목에 `(TEB)`가 없습니다(정본: *Integrated online trajectory planning and optimization in distinctive topologies*). 문맥으로는 병렬 위상 최적화까지만 지지되고, **"switching cost로 시간 일관성을 얻는다"** 와 **"선택을 commitment로 본다"** 는 원문보다 강합니다. ROS 구현에 `selection_cost_hysteresis`가 있으나 본문 서술보다 좁습니다.

### `williams2017mppi` — `main.tex:62`
MPPI 정본이 맞고 sampling-based MPC 근거로 적절하지만, **"본질적으로 매끄러운 해를 낸다"** 는 원문에 없는 수식(修飾)입니다. → smoothness 주장 삭제 또는 별도 출처.

### `helbing1995social` — `main.tex:64`
Social Force Model 정본이나, **로봇 freezing 완화**를 보인 논문이 아닙니다(로봇 항법을 다루지 않음). → freezing 주장에는 로봇 사회항법 문헌을 사용.

### `hall1966hidden` — `main.tex:64`, `main.tex:139`
제목의 `(proxemics)`는 편집자 설명구입니다. Hall은 **정성적·문화의존적 거리대**를 제시했을 뿐 **로봇 비용장**도 **전방 확장 비대칭 Gaussian 형상**도 정식화하지 않습니다. → 비용장·전방 비대칭은 Kirby 등 로보틱스/HRI 출처로 이관.

### `alahi2016sociallstm` — `main.tex:68`
Social-LSTM은 스텝마다 **단봉 이변량 Gaussian**을 냅니다. 이 문헌들을 묶어 **"multimodal 예측기"** 라 부르는 건 부정확합니다. → 문구를 "learning-based probabilistic predictors"로 바꾸거나, multimodal을 유지하려면 실제 multimodal 예측기로 교체.

### `riosmartinez2015survey` — `main.tex:139` (4개 인용 위치 중 1곳만 문제)
`main.tex:74`·`main.tex:407`·`main.tex:139`의 전방 확장 개인공간은 **모두 지지**됩니다. 다만 **후방 사각지대가 놀람·불편을 유발한다**는 주장은 이 서베이 본문에 없습니다. → 해당 문장만 출처 교체 또는 삭제.

### `cao2019dynamicchannel` — `main.tex:78`
저자 오류(`Z. Cao·K. Iba` → `C. Cao·S. Iba`, pp. 5551–5557). `main.tex:70`의 위상 통로 선택 근거는 지지되지만, `main.tex:78`의 **"시간적 통행측 일관성을 인식했다"** 는 원문보다 강합니다(50–100 Hz 재계획, 주기 간 commitment 미정의).

## FIX (12건) — 서지 교정

제목에 없는 괄호 설명구를 넣은 사례가 다수입니다: `(RVO/ORCA)`, `(H-signature)`, `(F-formation)`, `GCFF`, `(proxemics)`, `(TEB)`. arXiv 스타일에서 제목 필드에 설명을 섞으면 심사자가 서지 부주의로 읽습니다.

| 키 | 교정 |
|---|---|
| `vandenberg2011rvo` | 제목에서 `(RVO/ORCA)` 제거, Springer STAR 시리즈 vol. 70 보강 |
| `kirby2010thesis` | 제목 괄호 설명구 제거 |
| `bhattacharya2012topological` | 제목 괄호 제거. H-signature는 엄밀히 homology 불변량 |
| `dragan2013legibility` | `K. C. Lee` → `K. C. T. Lee` (이니셜 누락) |
| `trautman2010` | pp. 797–803 보강. freezing robot **용어**는 지지되나 본문이 제시한 구조적 원인까지는 아님 |
| `mavrogiannis2022momentum` | `J. Thomason` → `W. Thomason`. 각운동량 목적함수를 지속 증거 누적기처럼 묘사하지 말 것 |
| `francis2023principles` | 2023 preprint → 정본 ACM THRI **14(2), Art. 34, 2025** |
| `salzmann2020trajectron` | ECCV Part XVIII, LNCS vol. 12363 보강 |
| `kendon1990conducting` | `(F-formation)` 제거. o-space를 집단 외곽 경계와 동일시하거나 로봇의 no-split 규칙을 Kendon에 직접 귀속하지 말 것 |
| `setti2015gcff` | 제목에서 `GCFF` 제거. 소속·o-space 중심은 추정하나 **반경을 명시 출력하지는 않음** |
| `moussaid2009experimental` | 제목 괄호 제거. **N=40**은 실험실 성분이며 현장 성분은 보행자 2,670명 추적 — 본문이 N=40을 전체에 적용 |
| `degroot2024topology` | 연도 2025(vol. 41, pp. 110–126), `D. M. Gavrila`. **신규성 문장 축소**(위 최우선 조치) |

## KEEP (7건)

`mavrogiannis2023survey` · `gao2022evaluation` · `gupta2018socialgan` · `trautman2015dense` · `mavrogiannis2023winding` · `kuderer2012feature` · `kretzschmar2016irl`

## 미인용 서지 (`--uncited`)

- `macenski2020nav2` — 서지에 있으나 본문 `\cite` 없음. **판정: check.** Nav2 위에 올릴 결정 계층을 다루는 논문에서 Nav2 원논문이 미인용인 건 단순 정리 누락이라기보다 관련연구·구현 배치를 한 번 볼 대목입니다.

이 절은 탐지 전용이며 최상위 판정에 영향을 주지 않습니다.

## 미완 항목

**재컴파일 검증 미실시.** 작업 환경에 LaTeX 툴체인이 없어(`pdflatex` 부재) 프로토콜 Step 7(`Citation undefined` 경고 확인, 페이지 수 변화 확인)을 수행하지 못했습니다. 서지 교정 반영 후 별도 환경에서 컴파일 검증이 필요합니다.

## 산출물

- `CITATION_AUDIT.json` — 기계 판독용 판정 대장
- `.aris/traces/citation-audit/2026-08-10_run01/` — 배치별 심사 원문 5건 (항목마다 검증 URL 포함)
- `.aris/citation-audit/contexts.json`, `contexts.txt` — 추출된 (키, 인용 위치, 문맥) 원자료
