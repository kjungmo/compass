# compass_sim — COMPASS 최소 Gazebo Harmonic 테스트베드

COMPASS (Commit-and-Cross) 결정 컨트롤러(`compass_nav2::CompassController`)를
구동·점검하기 위한 최소 시뮬레이션 테스트베드입니다. 실내 사무실 세계와 2D
lidar 를 단 차동 구동 로봇, `ros_gz_bridge`, 반응형 보행자, 그리고 우리 플러그인을
적재하는 Nav2 `controller_server` 브링업으로 구성됩니다.

- **대상:** ROS 2 Jazzy + Gazebo Harmonic (`gz sim` 8.10)
- **세계:** 10 × 8 m 실내 (둘레 벽 + 기둥 1 + 선반 2)
- **로봇:** 차동 구동 + 360 빔 2D lidar + IMU
- **보행자:** 로봇이 약 2 m 안으로 들어오면 비켜서는 반응형 노드

## 구성 파일

| 경로 | 내용 |
|---|---|
| `worlds/compass_office.sdf` | Gazebo Harmonic 세계 (지면·태양·장애물·보행자 모델) |
| `urdf/diff_bot.urdf.xacro` | 차동 구동 로봇 + lidar + IMU (Harmonic 플러그인 포함) |
| `config/ros_gz_bridge.yaml` | `/cmd_vel`→sim, `/odom`·`/scan`·`/tf`·`ped/*`←sim 브리지 |
| `config/nav2_compass.yaml` | `FollowPath: compass_nav2::CompassController` + COMPASS 노브 |
| `launch/sim_bringup.launch.py` | gz sim + 브리지 + 스폰 + 보행자 + Nav2 브링업 |
| `scripts/reactive_pedestrian.py` | 반응형 보행자 노드 |

## 빌드

선행 조건: `compass_core`, `compass_nav2` 가 먼저 빌드되어 있어야 플러그인 적재가
가능합니다(같은 워크스페이스에서 함께 빌드하면 됩니다).

```bash
# 저장소 루트에서, ROS 2 Jazzy 환경을 source 한 셸 기준
colcon build --packages-select compass_core compass_nav2 compass_sim
source install/setup.bash
```

## Headless 실행 (WSL2 기본 권장)

```bash
source install/setup.bash
export MESA_GL_VERSION_OVERRIDE=3.3        # WSL2 등 소프트웨어 GL 환경 폴백
ros2 launch compass_sim sim_bringup.launch.py headless:=true
```

`headless:=true` (기본값)이면 `gz sim` 이 서버 전용(`-s`)으로 떠 렌더/GUI 가 없으므로
WSL2 GPU 스톨을 피합니다. GUI 가 필요하면 `headless:=false` 로 전환하십시오.

런치 인자:

| 인자 | 기본값 | 설명 |
|---|---|---|
| `headless` | `true` | `true` 면 `gz sim -s` (렌더 없음) |
| `use_nav2` | `true` | `controller_server` + `lifecycle_manager` 적재 |
| `use_pedestrian` | `true` | 반응형 보행자 노드 실행 |
| `world` | `compass_office.sdf` | 세계 SDF 경로 재정의 |
| `params_file` | `nav2_compass.yaml` | Nav2 파라미터 경로 재정의 |

## 스모크 점검 명령

런치를 띄운 뒤 다른 터미널에서(같은 env 활성화):

```bash
# 1) 컨트롤러 서버가 active 인지
ros2 lifecycle get /controller_server          # 기대: active

# 2) 적재된 플러그인 확인
ros2 param get /controller_server FollowPath.plugin   # 기대: compass_nav2::CompassController

# 3) 센서/오도메트리 토픽이 흐르는지
ros2 topic hz /scan
ros2 topic echo /odom --once

# 4) 보행자가 반응하는지
ros2 topic echo /ped/cmd_vel --once

# 5) /cmd_vel 산출 (목표를 줘서 FollowPath 를 구동했을 때)
ros2 topic echo /cmd_vel --once
```

`controller_server` 는 `FollowPath` 경로가 설정될 때(`computeVelocityCommands`)
`/cmd_vel` 을 산출합니다. 경로 없이 활성 상태만 점검하려면 (1)·(2) 로 충분합니다.

## 빈 복도 직진 검증 (경로 추종 PD)

직선 `FollowPath` 목표를 한 명령으로 주려면 `send_straight_goal` 을 씁니다.
이는 현재 `/odom` 위치에서 세계 +x 로 곧게 뻗는 경로를 만들어 `/follow_path`
액션으로 보냅니다 (`y_offset` 으로 경로를 횡으로 띄워 cross-track 보정을
시험할 수 있습니다).

```bash
# 빈 복도(보행자 없이) 직진 — 로봇이 곧게 가는지
ros2 launch compass_sim sim_bringup.launch.py headless:=true use_pedestrian:=false
ros2 run   compass_sim send_straight_goal                       # 다른 터미널
# 관찰: /odom y ≈ 일정, /cmd_vel angular.z ≈ 0 (위빙 없음)

# 횡오차 보정 — 경로를 +0.3 m 띄우면 매끄럽게 수렴(오버슈트 없음)해야 함
ros2 run compass_sim send_straight_goal --ros-args -p y_offset:=0.3
```

조향은 cross-track + heading PD(`pathTrackingAngularZ`, 과감쇠 기본값)라 직선
복도에서 위빙이 없습니다. 헤드리스 실측(ROS 2 Jazzy + Gazebo Harmonic, WSL2):

| 시나리오 | `/odom` 횡(y) | `/cmd_vel` angular.z | 판정 |
|---|---|---|---|
| 빈 복도 직진 | 편차 0.000 m | max\|ω\|=0.000, 부호전환 0 | 곧게 전진(+3.75 m) |
| 경로 +0.3 m 횡오차 | 0.000 → +0.292 m (오버슈트 0) | +0.9→0, 부호전환 0 | 매끄럽게 수렴 |
| 보행자 교차 | 횡 −0.133 m | −0.40(우측 커밋)→0 | 측면 커밋 보존(위빙 없이) |

> 예전 베어링 비례 조향은 감쇠가 없어 빈 복도에서도 ±0.8 m 위빙을 냈습니다
> (단위 테스트 `test_path_tracking` 가 회귀로 고정). 교차 시 한 번 측면을
> 커밋하고 유지하는 것이 COMPASS 의 시간 일관성(반-진동) 설계와 일치합니다.

## 세계만 빠르게 검증 (ROS 없이)

```bash
export MESA_GL_VERSION_OVERRIDE=3.3
gz sim -r -s --headless-rendering --iterations 200 sim/worlds/compass_office.sdf
```

플러그인 파일명(`gz-sim-*-system`)이 설치 Gazebo 와 맞지 않으면 여기서 실패합니다.

## WSL2 GPU 주의 (필독)

WSL2 환경에서는 배포판 `libGL` 과 호스트 드라이버가 충돌해 Gazebo 의
렌더링/lidar 가 느려지거나 멈출 수 있습니다. 따라서:

- 스모크는 **항상 `headless:=true`** 로 먼저 돌립니다(서버 전용, 렌더 없음).
- 그래도 lidar(`gpu_lidar`) 렌더가 막히면 `controller_server` 의 costmap 이
  관측을 못 받아 활성화가 지연될 수 있습니다. 이때
  `export MESA_GL_VERSION_OVERRIDE=3.3` (소프트웨어 폴백)을 적용하십시오.
- GUI(`headless:=false`)는 GPU 가 안정적으로 잡힐 때만 사용하십시오.

> 측정값(전환 감소율·지연·성공률 등)은 본 테스트베드 범위 밖입니다. 여기서는
> 빌드·정적 유효성·headless 스모크까지만 확인하며, 정량 실험은 후속 작업입니다.
