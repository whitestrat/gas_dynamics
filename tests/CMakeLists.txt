# Exactly the five required Toro cases, executed through the real JSON task.
add_test(NAME riemann.toro
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/run_toro.py"
        --bin-dir "$<TARGET_FILE_DIR:riemann_problem>"
        --output "${CMAKE_BINARY_DIR}/Testing/toro"
)
set_tests_properties(riemann.toro PROPERTIES TIMEOUT 120)
