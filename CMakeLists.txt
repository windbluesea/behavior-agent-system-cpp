cmake_minimum_required(VERSION 3.16)

project(behavior_agent_system_cpp VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BAS_BUILD_TESTS "Build tests" ON)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(bas_core
  src/agent_pipeline.cpp
  src/dis_adapter.cpp
  src/situation_fusion.cpp
  src/event_memory.cpp
  src/scenario_replay.cpp
  src/fire_control_engine.cpp
  src/maneuver_engine.cpp
  src/model_runtime.cpp
  src/decision_cache.cpp
)

target_include_directories(bas_core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(bas_demo src/main.cpp)
target_link_libraries(bas_demo PRIVATE bas_core)

add_executable(bas_replay src/replay_main.cpp)
target_link_libraries(bas_replay PRIVATE bas_core)

if(BAS_BUILD_TESTS)
  enable_testing()

  add_executable(test_memory tests/test_memory.cpp)
  target_link_libraries(test_memory PRIVATE bas_core)
  add_test(NAME test_memory COMMAND test_memory)

  add_executable(test_fire_control tests/test_fire_control.cpp)
  target_link_libraries(test_fire_control PRIVATE bas_core)
  add_test(NAME test_fire_control COMMAND test_fire_control)

  add_executable(test_maneuver tests/test_maneuver.cpp)
  target_link_libraries(test_maneuver PRIVATE bas_core)
  add_test(NAME test_maneuver COMMAND test_maneuver)

  add_executable(test_pipeline tests/test_pipeline.cpp)
  target_link_libraries(test_pipeline PRIVATE bas_core)
  add_test(NAME test_pipeline COMMAND test_pipeline)

  add_executable(test_replay_loader tests/test_replay_loader.cpp)
  target_link_libraries(test_replay_loader PRIVATE bas_core)
  add_test(NAME test_replay_loader COMMAND test_replay_loader)

  add_executable(test_replay_pipeline tests/test_replay_pipeline.cpp)
  target_link_libraries(test_replay_pipeline PRIVATE bas_core)
  add_test(NAME test_replay_pipeline COMMAND test_replay_pipeline)

  add_executable(test_latency_smoke tests/test_latency_smoke.cpp)
  target_link_libraries(test_latency_smoke PRIVATE bas_core)
  add_test(NAME test_latency_smoke COMMAND test_latency_smoke)

  add_test(NAME bas_demo_smoke COMMAND bas_demo)
  add_test(NAME bas_replay_smoke COMMAND bas_replay ../data/scenarios/demo_replay.bas)
endif()
