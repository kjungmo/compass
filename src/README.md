# src — 구현 (예정)

결정 계층 플래너 `Commit-and-Cross`의 구현이 들어갈 자리입니다.

계획: ROS2 `Nav2` 커스텀 컨트롤러(또는 플래너) 플러그인.
- 입력: `costmap`/laser(gap), 사람 추적 노드(track·ID)
- 주기 간 상태: 라벨 · `e^fwd` · `e^rev` · `ρ` · TTL (플러그인 멤버로 보존)
- 논문 §4.5 통합 알고리즘을 1:1로 구현

아직 비어 있습니다. 시뮬레이터(Gazebo/Isaac + PedSim 류) 구축이 R1~R4 실험의 선행 조건입니다.
