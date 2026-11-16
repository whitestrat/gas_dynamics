cmake_minimum_required(VERSION 3.21)
project(GasDynamics VERSION 0.4.0 LANGUAGES CXX)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Configure, build and run this project in Linux.")
endif()
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Use an out-of-source build: cmake --preset release")
endif()

find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
add_library(gd_riemann_exact INTERFACE)
add_library(gd::riemann_exact ALIAS gd_riemann_exact)
target_include_directories(gd_riemann_exact INTERFACE "${PROJECT_SOURCE_DIR}/include")
target_compile_features(gd_riemann_exact INTERFACE cxx_std_17)

# Bundled JSON parser: used only by the task, never by the mathematical solver.
add_library(gd_json INTERFACE)
target_include_directories(gd_json INTERFACE "${PROJECT_SOURCE_DIR}/third_party")

add_subdirectory(src/tasks/riemann)
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
