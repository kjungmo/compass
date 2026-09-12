# compass_sim — COMPASS 최소 Gazebo Harmonic 테스트베드

COMPASS (Commit-and-Cross) 결정 컨트롤러(`compass_nav2::CompassController`)를
구동·점검하기 위한 최소 시뮬레이션 테스트베드입니다. 실내 사무실 세계와 2D
lidar 를 단 차동 구동 로봇, `ros_gz_bridge`, 반응형 보행자, 그리고 우리 플러그인을
적재하는 Nav2 `controller_server` 브링업으로 구성됩니다.

현재 범위는 연구용 소프트웨어와 재현용 테스트베드입니다. 아래 주행 수치는
이전 기본 경로의 역사적 smoke 메모이며, 현재 소스의 재실행 결과가 아닙니다.
이번 검토에서 Gazebo/실물 로봇 주행은 실행하지 않았고, 새 measured-progress 및
candidate-trajectory 경로의 end-to-end 물리 성능은 아직 검증하지 않았습니다.

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
| `scripts/people_gt_publisher.py` | 보행자 odometry를 `/people` 입력으로 변환 |
| `scripts/send_straight_goal.py` | `/follow_path` 액션에 직선 경로 전송 |

## 빌드

선행 조건: ROS 2 Jazzy, Nav2, Gazebo Harmonic 및 `ros_gz` 의존성을 설치해야
합니다. `compass_msgs`, `compass_core`, `compass_nav2`도 필요하므로 새
워크스페이스에서는 의존성을 포함하는 `--packages-up-to`를 사용합니다.

```bash
# 저장소 루트에서, ROS 2 Jazzy 환경을 source 한 셸 기준
colcon build --base-paths src sim --packages-up-to compass_sim
source install/setup.bash
```

## Headless 실행 (WSL2 기본 권장)

```bash
source install/setup.bash
export MESA_GL_VERSION_OVERRIDE=3.3        # 이전 WSL2 환경의 GL 버전 호환 설정
ros2 launch compass_sim sim_bringup.launch.py headless:=true
```

`headless:=true` (기본값)이면 `gz sim` 이 서버 전용(`-s`)으로 떠 GUI를 열지
않습니다. `gpu_lidar` 센서는 여전히 렌더링 지원이 필요하므로 이것만으로 GPU
문제를 해결하지는 않습니다. GUI가 필요하면 `headless:=false`로 전환하십시오.

런치 인자:

| 인자 | 기본값 | 설명 |
|---|---|---|
| `headless` | `true` | `true` 면 `gz sim -s` (서버 전용, GUI 없음) |
| `use_nav2` | `true` | `controller_server` + `lifecycle_manager` 적재 |
| `use_pedestrian` | `true` | 반응형 보행자 노드 실행 |
| `world` | `compass_office.sdf` | 세계 SDF 경로 재정의 |
| `params_file` | `nav2_compass.yaml` | Nav2 파라미터 경로 재정의 |

## 연구용 선택 경로

기본 설정은 `FollowPath.use_measured_progress=false`,
`FollowPath.use_candidate_trajectories=false`, `FollowPath.k_rho=1.0`입니다.
측정 진행도를 켜도 responsiveness 노브가 자동으로 바뀌지 않습니다.
연구용 responsiveness 비교를 구성할 때는 별도 파라미터 파일에서
`k_rho=0.5`와 필요한 선택 경로를 명시하고 `params_file`로 전달하십시오.
노브는 controller 구성 시 읽으므로 동적 변경이 적용된다고 가정하지 마십시오.
모든 controller 모드에서 pose/plan 헤더는 costmap의 global frame과 같아야
합니다. 다르면 좌표를 자동 변환하지 않고 zero command를 반환합니다.
선택 경로를 모두 꺼도 legacy TTC 부호·미관측 costmap·L/R 방향·`a_brake*dt`
제동 수정은 적용됩니다. 기본 노브 유지가 이전 안전 결함의 유지를 뜻하지 않습니다.

측정 진행도 한계 `progress_max_gap=0.25 s`, `progress_max_speed=2.0 m/s`,
`progress_length=1.0 m`는 보정되지 않은 설정값입니다. 같은 XY 경로/프레임의
재발행은 진행 구간을 보존하지만 실제 경로 변경은 다시 시작합니다. 후보 궤적은
원형 footprint와 단순 보행자 예측에 제한되며 혼합 통과-측 클래스는 지원하지
않습니다. 계약과 물리 검증 경계는 [후보 궤적 문서](../docs/CANDIDATE_TRAJECTORIES.md)와
[진행도 문서](../src/compass_eval/RESPONSIVENESS.md)를 참조하십시오.

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
# 점검 목표: /odom y와 /cmd_vel angular.z를 기록하여 직진 동작 확인

# 횡오차 보정 — 경로를 +0.3 m 띄우면 매끄럽게 수렴(오버슈트 없음)해야 함
ros2 run compass_sim send_straight_goal --ros-args -p y_offset:=0.3
```

조향은 cross-track + heading PD(`pathTrackingAngularZ`)를 사용합니다. 다음은
이전 기본 경로의 headless smoke 관찰 메모(ROS 2 Jazzy + Gazebo Harmonic,
WSL2)입니다. 원본 주행 로그/정확한 소스 및 설정 해시가 함께 보존되어 있지 않아
현재 소스의 재현 증거나 일반적인 무위빙 보장으로 사용할 수 없습니다.

| 시나리오 | `/odom` 횡(y) | `/cmd_vel` angular.z | 판정 |
|---|---|---|---|
| 빈 복도 직진 | 편차 0.000 m | max\|ω\|=0.000, 부호전환 0 | 곧게 전진(+3.75 m) |
| 경로 +0.3 m 횡오차 | 0.000 → +0.292 m (오버슈트 0) | +0.9→0, 부호전환 0 | 매끄럽게 수렴 |
| 보행자 교차 | 횡 −0.133 m | −0.40(우측 커밋)→0 | 측면 커밋 보존(위빙 없이) |

> 단위 테스트 `test_path_tracking`는 소프트웨어 조향 회귀를 검사합니다.
> 이것은 위의 Gazebo 관찰을 재실행하거나 물리 안전성을 입증하는 테스트가 아닙니다.

## 세계만 빠르게 검증 (ROS 없이)

```bash
export MESA_GL_VERSION_OVERRIDE=3.3
gz sim -r -s --headless-rendering --iterations 200 sim/worlds/compass_office.sdf
```

플러그인 파일명(`gz-sim-*-system`)이 설치 Gazebo 와 맞지 않으면 여기서 실패합니다.

## WSL2 GPU 주의 (필독)

WSL2 환경에서는 배포판 `libGL` 과 호스트 드라이버가 충돌해 Gazebo 의
렌더링/lidar 가 느려지거나 멈출 수 있습니다. 따라서:

- 스모크는 `headless:=true`로 먼저 점검합니다(서버 전용, GUI 없음).
- 그래도 lidar(`gpu_lidar`) 렌더가 막히면 `controller_server` 의 costmap 이
  관측을 못 받아 활성화가 지연될 수 있습니다. 이때
  GL/센서 오류 로그를 확인하십시오. 위의 `MESA_GL_VERSION_OVERRIDE`는
  GL 버전 호환 설정이며 소프트웨어 렌더러를 선택하거나 성공을 보장하지 않습니다.
- GUI(`headless:=false`)는 GPU 가 안정적으로 잡힐 때만 사용하십시오.

> 현재 변경의 검증은 소프트웨어 빌드/테스트 범위입니다. 역사적 smoke와 구분하여
> 새 소스의 bringup을 재실행하고 로그를 보존해야 합니다. 성공률·충돌률·물리
> freezing·사회 거리·사람이 읽는 움직임은 후속 정량 실험의 대상입니다.
