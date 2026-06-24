cmake_minimum_required(VERSION 3.8)
project(compass_eval)
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
find_package(ament_cmake REQUIRED)
find_package(compass_core REQUIRED)

add_executable(compass_eval src/main.cpp)
target_include_directories(compass_eval PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
ament_target_dependencies(compass_eval compass_core)

install(TARGETS compass_eval DESTINATION lib/${PROJECT_NAME})
ament_package()
