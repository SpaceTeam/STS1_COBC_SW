function(add_golden_test test_file)
    if(NOT ("${test_file}" MATCHES "\.test\.cpp$"))
        message(FATAL_ERROR "Name of test file `${test_file}` does not end with .test.cpp")
    endif()

    get_filename_component(test_file_name ${test_file} NAME_WE)
    set(target ${PROJECT_NAME}_${test_file_name})

    add_executable(${target} EXCLUDE_FROM_ALL)
    target_sources(${target} PRIVATE "${test_file}")
    target_include_directories(${target} ${warning_guard} PUBLIC "${CMAKE_SOURCE_DIR}")
    set_target_properties(${target} PROPERTIES OUTPUT_NAME ${test_file_name})

    # Check if an expected output file exists for the current build type
    set(expected_output_directory "${CMAKE_SOURCE_DIR}/Tests/GoldenTests/ExpectedOutputs")
    set(expected_output_file
        "${expected_output_directory}/${test_file_name}_${CMAKE_BUILD_TYPE}.txt"
    )
    if(NOT EXISTS ${expected_output_file})
        # If not, use the default expected output file
        set(expected_output_file "${expected_output_directory}/${test_file_name}.txt")
    endif()

    add_test(
        NAME ${test_file_name}
        COMMAND
            "${CMAKE_COMMAND}" -D "TEST_EXECUTABLE=$<TARGET_FILE:${target}>" -D
            "EXPECTED_OUTPUT_FILE=${expected_output_file}" -P
            "${CMAKE_SOURCE_DIR}/cmake/run-golden-test.cmake"
    )
    # Set a timeout in case we have deadlocks, infinite loops, etc.
    set_tests_properties(${test_file_name} PROPERTIES TIMEOUT 8)
endfunction()
