cmake_minimum_required(VERSION 3.16)
project(gasdynamics LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(riemann_problem src/tasks/riemann_problem.cpp)
set_target_properties(riemann_problem PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)
    target_compile_options(riemann_problem PRIVATE /W4)
else()
    target_compile_options(riemann_problem PRIVATE -Wall -Wextra -Wpedantic)
endif()