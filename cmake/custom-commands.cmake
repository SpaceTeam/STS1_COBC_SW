function(find_package_and_notify package_name)
    find_package(${package_name} REQUIRED)
    get_target_property(${package_name}_include_dirs ${package_name} INTERFACE_INCLUDE_DIRECTORIES)
    message("Found ${package_name} include dirs: ${${package_name}_include_dirs}")
endfunction()

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
