# COMPASS R3 — per-cycle worst-case latency (2^K 무가지치기, 전 class 안전)
CPU: AMD Ryzen 5 5500GT with Radeon Graphics
측정: DecisionCore::step (열거+비용적분 J+안전판정+결정), 20000 iters/조건.

| 관련자 K | 명시 열거 2^K | 평균(us) | p50(us) | p99(us) | 최대(us) | 20Hz 예산(50ms) 점유 |
|---|---|---|---|---|---|---|
| 1 | 2 | 1.568 | 1.523 | 2.595 | 153.607 | 0.3072% |
| 2 | 4 | 5.429 | 5.160 | 7.885 | 356.086 | 0.7122% |
| 3 | 8 | 15.918 | 14.527 | 58.642 | 651.257 | 1.3025% |

(p99/max << 50 ms 주기 예산 -> 20 Hz 실시간 여유 확인. |S_k| 가지치기 후 분포는 실 costmap 의존 [TBD].)
