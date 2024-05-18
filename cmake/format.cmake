set(fix_flag "")
if(FIX)
    set(fix_flag "-D FIX=YES")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" "${fix_flag}" -P cmake/format-cpp.cmake
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" RESULT_VARIABLE cpp_result
)

execute_process(
    COMMAND "${CMAKE_COMMAND}" "${fix_flag}" -P cmake/format-cmake.cmake
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" RESULT_VARIABLE cmake_result
)

if(NOT cpp_result EQUAL "0" OR NOT cmake_result EQUAL "0")
    message(FATAL_ERROR "Some files are badly formatted")
endif()
