macro(add_golden_test)
    set(options "")
    set(one_value_args FILE)
    set(multi_value_args LIB)
    cmake_parse_arguments(GT "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    get_filename_component(test_filename ${GT_FILE} NAME_WE)
    set(target_name ${PROJECT_NAME}_${test_filename})

    add_executable(${target_name} EXCLUDE_FROM_ALL ${GT_FILE})
    set_target_properties(${target_name} PROPERTIES OUTPUT_NAME ${test_filename})
    target_link_libraries(${target_name} PUBLIC ${GT_LIB})

    add_custom_command(
        OUTPUT "${test_filename}.output"
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/TestRunner.sh"
                $<TARGET_FILE:${target_name}>
        DEPENDS ${target_name} Scripts/TestRunner.sh
    )

    list(APPEND output_files "${test_filename}.output")

    add_custom_target(
        ${target_name}_Clean COMMAND ${CMAKE_COMMAND} -E remove -f "${test_filename}.output"
    )

    add_test(NAME ${test_filename}_Test
             COMMAND diff "${test_filename}.output"
                     "${CMAKE_CURRENT_SOURCE_DIR}/ExpectedOutputs/${test_filename}.txt"
    )
endmacro()
