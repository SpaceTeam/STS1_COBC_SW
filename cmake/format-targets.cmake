add_custom_target(
    format-check
    COMMAND "${CMAKE_COMMAND}"
    -P "${PROJECT_SOURCE_DIR}/cmake/format.cmake"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMENT "Checking code format"
    VERBATIM
)

add_custom_target(
    format-fix
    COMMAND "${CMAKE_COMMAND}"
    -D FIX=YES
    -P "${PROJECT_SOURCE_DIR}/cmake/format.cmake"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMENT "Fixing code format"
    VERBATIM
)
