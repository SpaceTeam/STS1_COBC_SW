# Must be a macro because otherwise CMAKE_MODULE_PATH, that is set by find_package(Catch2), is not
# propagated upwards, i.e., as soon as the function ends CMAKE_MODULE_PATH would be unset again.
macro(find_package_and_notify package_name)
    get_property(
        imported_targets_before
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        PROPERTY IMPORTED_TARGETS
    )
    find_package(${package_name} REQUIRED)
    get_property(
        imported_targets
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        PROPERTY IMPORTED_TARGETS
    )
    list(REMOVE_ITEM imported_targets ${imported_targets_before})
    foreach(target IN LISTS imported_targets)
        get_target_property(include_dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
        message("Found ${target}: include dirs = ${include_dirs}")
    endforeach()
endmacro()

function(find_rodos)
    set(RODOS_PACKAGE_NAME
        "rodos"
        CACHE STRING "Name of the Rodos package used when calling find_package()"
    )
    find_package(${RODOS_PACKAGE_NAME} REQUIRED ${ARGN})
    get_target_property(rodos_location rodos::rodos LOCATION)
    message("Found rodos: ${rodos_location}")
endfunction()

function(add_program program_name)
    add_executable(CobcSw_${program_name} ${ARGN})
    set_target_properties(CobcSw_${program_name} PROPERTIES OUTPUT_NAME ${program_name})

    if(CMAKE_SYSTEM_NAME STREQUAL Generic)
        # Automatically call objcopy on the executable targets after the build
        objcopy_target(CobcSw_${program_name})
    endif()
endfunction()

function(objcopy_target target_name)
    get_target_property(output_name ${target_name} OUTPUT_NAME)
    add_custom_command(
        TARGET ${target_name}
        POST_BUILD
        COMMAND "${CMAKE_OBJCOPY}" -O binary ${output_name} ${output_name}.bin
        BYPRODUCTS ${output_name}.bin
        COMMENT "Calling objcopy on ${output_name} to generate flashable ${output_name}.bin"
        VERBATIM
    )
endfunction()

macro(add_golden_test)
    set(options "")
    set(oneValueArgs FILE)
    set(multiValueArgs LIB)
    cmake_parse_arguments(GT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Make test file relative to the Test directory
    cmake_path(RELATIVE_PATH GT_FILE BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    OUTPUT_VARIABLE GT_FILE)

    get_filename_component(filename ${GT_FILE} NAME_WE)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" test_name ${GT_FILE})

    add_executable("${test_name}_Bin" EXCLUDE_FROM_ALL ${GT_FILE})
    target_link_libraries("${test_name}_Bin" PUBLIC ${GT_LIB})

    add_custom_command(
        OUTPUT
        "${test_name}_Bin.output"
        COMMAND
        bash
        ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/TestRunner.sh
        $<TARGET_FILE:${test_name}_Bin>
        DEPENDS
        ${test_name}_Bin Scripts/TestRunner.sh)

    list(APPEND output_files "${test_name}_Bin.output")

    add_custom_target(${test_name}-clean
        COMMAND
        ${CMAKE_COMMAND} -E remove -f
        "${test_name}_Bin.output")

    add_test(
        NAME
        ${filename}_Test
        COMMAND
        diff ${test_name}_Bin.output ${CMAKE_CURRENT_SOURCE_DIR}/ExpectedOutputs/${filename}.txt
    )
endmacro()
