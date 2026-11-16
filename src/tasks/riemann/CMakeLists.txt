add_executable(riemann_problem main.cpp)
target_link_libraries(riemann_problem PRIVATE gd::riemann_exact gd_json)
set_target_properties(riemann_problem PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(riemann_problem PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_custom_target(export_toro
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/run_toro.py"
        --bin-dir "$<TARGET_FILE_DIR:riemann_problem>"
        --output "${PROJECT_SOURCE_DIR}/results/toro"
    DEPENDS riemann_problem
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    USES_TERMINAL VERBATIM
)
add_custom_target(plots
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/plot_results.py"
        "${PROJECT_SOURCE_DIR}/results/toro"
    DEPENDS export_toro
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    USES_TERMINAL VERBATIM
)
