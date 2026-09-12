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
if(BUILD_TESTING)
  enable_testing()
  add_executable(test_observer test/test_observer.cpp)
  target_include_directories(test_observer PRIVATE include)
  ament_target_dependencies(test_observer compass_core)
  add_test(NAME observer_regression COMMAND test_observer)
endif()
ament_package()
