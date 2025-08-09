cmake_minimum_required(VERSION 3.20)
project(rectifier_can_host_tests C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# App under test
add_library(app_obj ../src/app/rectifier.c)
target_include_directories(app_obj PUBLIC ../src)

# Stub CAN IF for host
add_library(can_stub can_if_stub.c)
target_include_directories(can_stub PUBLIC ../src)

# Unit tests with GoogleTest
include(FetchContent)
FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(unit_tests test_rectifier_id.cpp test_scaling.cpp)
target_include_directories(unit_tests PUBLIC ../src)
target_link_libraries(unit_tests PRIVATE app_obj can_stub gtest gtest_main)

add_test(NAME unit_tests COMMAND unit_tests)
