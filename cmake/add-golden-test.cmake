# Requires ${test_name}.test.cpp and ExpectedOutputs/${test_name}.txt to exist
function(add_golden_test test_name)
    if(NOT (CMAKE_SYSTEM_NAME STREQUAL Linux))
        message(FATAL_ERROR "Golden tests are only supported on Linux")
    endif()
    set(test_source_file "${CMAKE_CURRENT_LIST_DIR}/${test_name}.test.cpp")
    if(NOT EXISTS ${test_source_file})
        message(FATAL_ERROR "Test source file ${test_source_file} does not exist")
    endif()

    set(target ${PROJECT_NAME}_${test_name})
    add_executable(${target} "${test_source_file}")
    target_include_directories(${target} ${warning_guard} PUBLIC "${CMAKE_SOURCE_DIR}")
    set_target_properties(${target} PROPERTIES OUTPUT_NAME ${test_name}Test)

    get_filename_component(test_directory ${test_source_file} DIRECTORY)
    set(expected_output_file "${test_directory}/ExpectedOutputs/${test_name}.txt")
    add_test(
        NAME ${test_name}
        COMMAND
            "${CMAKE_COMMAND}" #
            -D "TEST_EXECUTABLE=$<TARGET_FILE:${target}>" #
            -D "EXPECTED_OUTPUT_FILE=${expected_output_file}" #
            -D "BUILD_CONFIG=$<CONFIG>" #
            -P "${CMAKE_SOURCE_DIR}/cmake/run-golden-test.cmake"
    )
    # Set a timeout in case we have deadlocks, infinite loops, etc.
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 8)
endfunction()
