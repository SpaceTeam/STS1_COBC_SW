# Must be a macro because otherwise CMAKE_MODULE_PATH, that is set by find_package(Catch2), is not
# propagated upwards, i.e., as soon as the function ends CMAKE_MODULE_PATH would be unset again.
macro(find_package_and_notify package_name)
    get_property(
        imported_targets_before DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS
    )
    find_package(${package_name} REQUIRED)
    get_property(imported_targets DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
    list(REMOVE_ITEM imported_targets ${imported_targets_before})
    foreach(target IN LISTS imported_targets)
        get_target_property(include_dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
        message("Found ${target}: include dirs = ${include_dirs}")
    endforeach()
endmacro()

function(find_rodos)
    set(RODOS_PACKAGE_NAME "rodos"
        CACHE STRING "Name of the Rodos package used when calling find_package()"
    )
    find_package(${RODOS_PACKAGE_NAME} REQUIRED ${ARGN})
    get_target_property(rodos_location rodos::rodos LOCATION)
    message("Found rodos: ${rodos_location}")
endfunction()

function(add_program program_name)
    set(target ${PROJECT_NAME}_${program_name})
    add_executable(${target} ${ARGN})
    set_target_properties(${target} PROPERTIES OUTPUT_NAME ${program_name})
    if(CMAKE_SYSTEM_NAME STREQUAL Generic)
        target_link_options(${target} PRIVATE -T "${linker_script}")
        set_target_properties(${target} PROPERTIES LINK_DEPENDS "${linker_script}")
        set_target_properties(${target} PROPERTIES SUFFIX ".elf")
        # Automatically call objcopy on the executable targets after the build
        objcopy_target(${target})
        if(BUILD_FOR_USE_WITH_BOOTLOADER)
            add_metadata(${target})
        endif()
    endif()
endfunction()

function(add_test_program test_name)
    set(test_source_file "${CMAKE_CURRENT_LIST_DIR}/${test_name}.test.cpp")
    if(NOT EXISTS ${test_source_file})
        message(FATAL_ERROR "Test source file ${test_source_file} does not exist")
    endif()

    set(target ${PROJECT_NAME}_${test_name})
    add_executable(${target} ${test_source_file})
    set_target_properties(${target} PROPERTIES OUTPUT_NAME ${test_name}Test)
    if(CMAKE_SYSTEM_NAME STREQUAL Generic)
        target_link_options(${target} PRIVATE -T "${linker_script}")
        set_target_properties(${target} PROPERTIES LINK_DEPENDS "${linker_script}")
        set_target_properties(${target} PROPERTIES SUFFIX ".elf")
        # Automatically call objcopy on the executable targets after the build
        objcopy_target(${target})
        if(BUILD_FOR_USE_WITH_BOOTLOADER)
            add_metadata(${target})
        endif()
    endif()
endfunction()

function(objcopy_target target)
    get_target_property(output_name ${target} OUTPUT_NAME)
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:${target}>"
                "$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.bin"
        COMMENT "Calling objcopy on ${output_name} to generate flashable ${output_name}.bin"
        VERBATIM
    )
endfunction()

function(add_metadata target)
    get_target_property(output_name ${target} OUTPUT_NAME)
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND
            "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../Scripts/add_metadata.py"
            "$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.bin"
        COMMENT "Adding metadata to ${output_name}.bin"
        VERBATIM
    )
endfunction()

function(all_targets_include_directories include_directories)
    get_property(target_names DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
    message("Setting include directory to ${include_directories} for targets:")
    foreach(target IN LISTS target_names)
        message("- ${target}")
        get_target_property(type ${target} TYPE)
        if(type STREQUAL INTERFACE_LIBRARY)
            target_include_directories(${target} INTERFACE ${include_directories})
        else()
            target_include_directories(${target} PUBLIC ${include_directories})
        endif()
    endforeach()
endfunction()
