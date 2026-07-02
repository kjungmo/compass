cmake_minimum_required(VERSION 3.16)
project(compass_sim)

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclpy REQUIRED)

# 데이터 자산(세계/로봇/런치/설정) 설치.
install(
  DIRECTORY worlds urdf launch config
  DESTINATION share/${PROJECT_NAME}
)

# 반응형 보행자 노드(파이썬) 설치.
install(
  PROGRAMS scripts/reactive_pedestrian.py
  DESTINATION lib/${PROJECT_NAME}
  RENAME reactive_pedestrian
)

# 보행자 지상 진실 /people 발행 노드(파이썬) 설치.
install(
  PROGRAMS scripts/people_gt_publisher.py
  DESTINATION lib/${PROJECT_NAME}
  RENAME people_gt_publisher
)

# 직선 FollowPath 목표 송신기(파이썬) — 빈 복도 직진 스모크 재현용.
install(
  PROGRAMS scripts/send_straight_goal.py
  DESTINATION lib/${PROJECT_NAME}
  RENAME send_straight_goal
)

ament_package()
