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
    add_executable(${PROJECT_NAME}_${program_name} ${ARGN})
    set_target_properties(${PROJECT_NAME}_${program_name} PROPERTIES OUTPUT_NAME ${program_name})

    # Add a definition of the target system
    if(CMAKE_SYSTEM_NAME STREQUAL Generic)
	    target_compile_definitions(${PROJECT_NAME}_${program_name} PUBLIC GENERIC_SYSTEM) 
        message("Adding -DGENERIC_SYSTEM to ${PROJECT_NAME}_${program_name}")
    elseif(CMAKE_SYSTEM_NAME STREQUAL Linux)
	    target_compile_definitions(${PROJECT_NAME}_${program_name} PUBLIC LINUX_SYSTEM)
        message("Adding -DLINUX_SYSTEM to ${PROJECT_NAME}_${program_name}")
    else()
	    message(SEND_ERROR "CMAKE_SYSTEM_NAME is neither Generic nor Linux")
    endif()


    if(CMAKE_SYSTEM_NAME STREQUAL Generic)
        # Automatically call objcopy on the executable targets after the build
        objcopy_target(${PROJECT_NAME}_${program_name})
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

function(all_targets_include_directories include_directories)
    get_property(
        target_names
        DIRECTORY ${PROJECT_SOURCE_DIR}
        PROPERTY BUILDSYSTEM_TARGETS
    )
    message("Setting include directory to ${include_directories} for targets:")
    foreach(target IN LISTS target_names)
        message("- ${target}")
        get_target_property(type ${target} TYPE)
        if(type STREQUAL INTERFACE_LIBRARY)
            target_include_directories(${target} ${warning_guard} INTERFACE ${include_directories})
        else()
            target_include_directories(${target} ${warning_guard} PUBLIC ${include_directories})
        endif()
    endforeach()
endfunction()
