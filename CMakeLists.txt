cmake_minimum_required(VERSION 3.10)
project(SensorGuard VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(Threads REQUIRED)

add_library(sg_core STATIC
  src/types.cpp src/detectors.cpp src/state_machine.cpp src/monitor.cpp
  src/simulated_source.cpp src/device_source.cpp src/logger.cpp)
target_include_directories(sg_core PUBLIC include PRIVATE driver)
target_link_libraries(sg_core PUBLIC Threads::Threads)

add_executable(sensorguardd src/main.cpp)
target_link_libraries(sensorguardd sg_core)

add_executable(sgctl src/sgctl.cpp)
target_link_libraries(sgctl sg_core)

enable_testing()
add_executable(sg_tests
  tests/test_main.cpp tests/test_detectors.cpp tests/test_state_machine.cpp
  tests/test_queue.cpp tests/test_sources.cpp tests/test_monitor.cpp)
target_link_libraries(sg_tests sg_core)
add_test(NAME unit_and_integration COMMAND sg_tests)
add_test(NAME e2e_simulation COMMAND ${CMAKE_SOURCE_DIR}/scripts/e2e_sim.sh $<TARGET_FILE:sensorguardd>)
