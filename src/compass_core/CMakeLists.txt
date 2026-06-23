cmake_minimum_required(VERSION 3.8)
project(compass_core)
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
find_package(ament_cmake REQUIRED)
add_library(compass_core
  src/version.cpp
  src/topo_class.cpp
  src/accumulator.cpp
  src/tie_breaker.cpp
  src/safety.cpp
  src/cost_evaluator.cpp
  src/decision_core.cpp)
target_include_directories(compass_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
install(DIRECTORY include/ DESTINATION include)
install(TARGETS compass_core EXPORT export_compass_core
  ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
ament_export_targets(export_compass_core HAS_LIBRARY_TARGET)
if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_smoke test/test_smoke.cpp)
  target_link_libraries(test_smoke compass_core)
  ament_add_gtest(test_topo_class test/test_topo_class.cpp)
  target_link_libraries(test_topo_class compass_core)
  ament_add_gtest(test_state test/test_state.cpp)
  target_link_libraries(test_state compass_core)
  ament_add_gtest(test_accumulator test/test_accumulator.cpp)
  target_link_libraries(test_accumulator compass_core)
  ament_add_gtest(test_tie_breaker test/test_tie_breaker.cpp)
  target_link_libraries(test_tie_breaker compass_core)
  ament_add_gtest(test_safety test/test_safety.cpp)
  target_link_libraries(test_safety compass_core)
  ament_add_gtest(test_cost test/test_cost.cpp)
  target_link_libraries(test_cost compass_core)
  ament_add_gtest(test_decision_core test/test_decision_core.cpp)
  target_link_libraries(test_decision_core compass_core)
endif()
ament_package()
