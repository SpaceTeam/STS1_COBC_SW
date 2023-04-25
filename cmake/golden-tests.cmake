macro(add_golden_test)
    # Parse arguments
    set(options "")
    set(one_value_args TESTFILE)
    set(multi_value_args SOURCE LIB)
    cmake_parse_arguments(GT "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    # Retrieve file name
    get_filename_component(test_filename ${GT_TESTFILE} NAME_WE)
    set(target_name ${PROJECT_NAME}_${test_filename})

    # Sanity check
    if(NOT ("${GT_TESTFILE}" MATCHES "\.test\.cpp$")) 
        MESSAGE(FATAL_ERROR "Name of test file `${GT_TESTFILE}` do not end with .test.cpp")
    endif()

    # Create executable
    add_executable(${target_name} EXCLUDE_FROM_ALL ${GT_TESTFILE} ${GT_SOURCE})
    target_include_directories(${target_name} ${warning_guard} PUBLIC "${CMAKE_SOURCE_DIR}")
    set_target_properties(${target_name} PROPERTIES OUTPUT_NAME ${test_filename})
    target_link_libraries(${target_name} PUBLIC ${GT_LIB})

    # Create output dependency (used in Tests/GoldenTests/CMakeLists.txt)
    add_custom_command(
        OUTPUT "${test_filename}.output"
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/TestRunner.sh"
                $<TARGET_FILE:${target_name}>
        DEPENDS ${target_name} Scripts/TestRunner.sh
    )
    list(APPEND output_files "${test_filename}.output")

    # Create clean target
    add_custom_target(
        ${target_name}_Clean COMMAND ${CMAKE_COMMAND} -E remove -f "${test_filename}.output"
    )

    # Create test
    add_test(NAME ${test_filename}_Test
             COMMAND diff "${test_filename}.output"
                     "${CMAKE_CURRENT_SOURCE_DIR}/ExpectedOutputs/${test_filename}.txt")
    
endmacro()


# Used to test real threads
macro(add_thread_golden_test)
    set(options "")
    set(multi_value_args FILES LIB)
    cmake_parse_arguments(GT "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    list(GET GT_FILES 0 test_filename)
    get_filename_component(test_filename ${test_filename} NAME_WE)

    set(target_name ${PROJECT_NAME}_${test_filename})

    add_executable(${target_name} EXCLUDE_FROM_ALL ${GT_FILES})
    target_include_directories(${target_name} ${warning_guard} PUBLIC "${CMAKE_SOURCE_DIR}")
    set_target_properties(${target_name} PROPERTIES OUTPUT_NAME ${test_filename})
    target_link_libraries(${target_name} PUBLIC ${GT_LIB})

    add_custom_command(
        OUTPUT "${test_filename}.output"
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/ThreadTestRunner.sh"
                $<TARGET_FILE:${target_name}>
        DEPENDS ${target_name} Scripts/ThreadTestRunner.sh
    )

    list(APPEND thread_output_files "${test_filename}.output")

    add_custom_target(
        ${target_name}_Clean COMMAND ${CMAKE_COMMAND} -E remove -f "${test_filename}.output"
    )

    add_test(NAME ${test_filename}_Test
             COMMAND diff "${test_filename}.output.trimmed"
                     "${CMAKE_CURRENT_SOURCE_DIR}/ExpectedOutputs/${test_filename}.txt"
    )
endmacro()
