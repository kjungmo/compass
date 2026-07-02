cmake_minimum_required(VERSION 3.10)
project(compass_nav2)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

find_package(ament_cmake REQUIRED)
find_package(compass_core REQUIRED)
find_package(compass_msgs REQUIRED)
find_package(nav2_core REQUIRED)
find_package(nav2_costmap_2d REQUIRED)
find_package(nav2_util REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)

set(dependencies
  compass_msgs
  nav2_core
  nav2_costmap_2d
  nav2_util
  pluginlib
  rclcpp
  rclcpp_lifecycle
  geometry_msgs
  nav_msgs
  tf2
  tf2_ros)

# =============================================================================
# compass_controller: nav2_core::Controller 플러그인 라이브러리.
# 결정 로직은 compass_core 에 있고, 이 라이브러리는 costmap/pose/people 을 코어
# 입력으로 사상하는 얇은 어댑터다. 라이브러리 이름은 플러그인 XML 의
# <library path="compass_controller"> 와 일치해야 한다.
# =============================================================================
add_library(compass_controller SHARED
  src/compass_controller.cpp
  src/costmap_env_query.cpp
  src/people_conversion.cpp
  src/path_tracking.cpp)
target_include_directories(compass_controller PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
ament_target_dependencies(compass_controller ${dependencies})
target_link_libraries(compass_controller compass_core::compass_core)

install(TARGETS compass_controller
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
install(DIRECTORY include/
  DESTINATION include)
install(DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config)

# nav2_core 의 컨트롤러 서버가 로드하는 플러그인 description 을 export.
pluginlib_export_plugin_description_file(nav2_core compass_controller_plugin.xml)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)

  # 런타임 스모크: 플러그인을 pluginlib 로 로드 (설치 필요).
  ament_add_gtest(test_plugin_load test/test_plugin_load.cpp)
  ament_target_dependencies(test_plugin_load pluginlib nav2_core)

  # toPersons 변환 단위 테스트 (node 불필요, 좌표·속도 규약 회귀).
  ament_add_gtest(test_people_conversion test/test_people_conversion.cpp)
  target_link_libraries(test_people_conversion compass_controller)
  ament_target_dependencies(test_people_conversion
    compass_msgs compass_core tf2_ros)

  # 경로 추종 PD 조향 단위 테스트 (node 불필요): 직선 정렬 시 ω≈0, 횡오차
  # 시 경로 쪽으로 위빙 없이 수렴, side_bias 부호 보존을 회귀로 고정한다.
  ament_add_gtest(test_path_tracking test/test_path_tracking.cpp)
  target_link_libraries(test_path_tracking compass_controller)
  ament_target_dependencies(test_path_tracking compass_core)
endif()

ament_export_include_directories(include)
ament_export_libraries(compass_controller)
ament_export_dependencies(compass_core ${dependencies})

ament_package()
