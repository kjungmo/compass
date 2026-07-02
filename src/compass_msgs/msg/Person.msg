# 추적된 사람 한 명의 지상 진실/추정 상태 (compass_core::Person 와 1:1).
# 프레임: 메시지를 담는 People.header.frame_id 좌표계에서의 평면(2D) 상태.
uint64 id          # 안정적 식별자 (이름 해시 등)
float64 x          # 위치 x (m)
float64 y          # 위치 y (m)
float64 theta      # 헤딩 (rad), 진행 방향
float64 vx         # 월드 프레임 속도 x 성분 (m/s)
float64 vy         # 월드 프레임 속도 y 성분 (m/s)
float64[4] covariance  # 위치 공분산 2x2, 행 우선 [c00, c01, c10, c11]
