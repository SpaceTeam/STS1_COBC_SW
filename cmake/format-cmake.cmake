cmake_minimum_required(VERSION 3.14)

macro(default name)
    if(NOT DEFINED "${name}")
        set("${name}" "${ARGN}")
    endif()
endmacro()

default(FIX NO)
if(FIX)
    set(flag -i)
else()
    set(flag --check)
endif()

# GLOB_RECURSE needs a directory so we have to manually add the top-level CMakeLists.txt
file(GLOB_RECURSE files cmake/*.cmake Sts1CobcSw/CMakeLists.txt Tests/CMakeLists.txt)
file(GLOB top_level_cml_file CMakeLists.txt)
list(APPEND files "${top_level_cml_file}")
set(badly_formatted "")
set(output "")
string(LENGTH "${CMAKE_SOURCE_DIR}/" path_prefix_length)

foreach(file IN LISTS files)
    execute_process(
        COMMAND cmake-format "${flag}" "${file}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE result
        ERROR_VARIABLE error_output
    )
    if(NOT FIX AND NOT (result EQUAL "0" OR result EQUAL "1"))
        message("${error_output}")
        message(FATAL_ERROR "'${file}': formatter returned with ${result}")
    endif()
    if(NOT FIX AND result EQUAL "1")
        string(SUBSTRING "${file}" "${path_prefix_length}" -1 relative_file)
        list(APPEND badly_formatted "${relative_file}")
    endif()
    set(error_output "")
endforeach()

if(NOT badly_formatted STREQUAL "")
    list(JOIN badly_formatted "\n" bad_list)
    message("The following files are badly formatted:\n\n${bad_list}\n")
    message(FATAL_ERROR "Run again with FIX=YES to fix these files.")
endif()
