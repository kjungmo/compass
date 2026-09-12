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

add_executable(compass_trace src/trace_main.cpp)
target_include_directories(compass_trace PRIVATE include)
ament_target_dependencies(compass_trace compass_core)

if(BUILD_TESTING)
  enable_testing()
  add_executable(compass_response_test test_response.cpp)
  target_include_directories(compass_response_test PRIVATE include)
  ament_target_dependencies(compass_response_test compass_core)
  add_test(NAME compass_response_contract COMMAND compass_response_test)
  add_executable(compass_trace_test test_trace.cpp)
  target_include_directories(compass_trace_test PRIVATE include)
  ament_target_dependencies(compass_trace_test compass_core)
  add_test(NAME compass_trace_state_equivalence COMMAND compass_trace_test)
endif()

install(TARGETS compass_eval compass_trace DESTINATION lib/${PROJECT_NAME})
ament_package()
