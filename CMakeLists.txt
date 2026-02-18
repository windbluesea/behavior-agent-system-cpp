cmake_minimum_required(VERSION 3.16)

project(behavior_agent_system_cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(bas_core
  src/dis_adapter.cpp
  src/situation_fusion.cpp
  src/event_memory.cpp
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

enable_testing()
add_test(NAME bas_demo_smoke COMMAND bas_demo)
