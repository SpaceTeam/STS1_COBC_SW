execute_process(
    COMMAND "${CMAKE_COMMAND}" -P cmake/format-cpp.cmake
    COMMAND "${CMAKE_COMMAND}" -P cmake/format-cmake.cmake
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)