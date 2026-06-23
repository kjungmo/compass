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

ament_package()
