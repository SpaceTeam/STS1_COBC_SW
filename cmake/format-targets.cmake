add_custom_target(
    format-check
    COMMAND "${CMAKE_COMMAND}"
    -P "${PROJECT_SOURCE_DIR}/cmake/format.cmake"
    -D "PATTERNS=${PATTERNS}"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMENT "Checking code format"
    VERBATIM
)

add_custom_target(
    format-fix
    COMMAND "${CMAKE_COMMAND}"
    -P "${PROJECT_SOURCE_DIR}/cmake/format.cmake"
    -D "PATTERNS=${PATTERNS}"
    -D FIX=YES
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMENT "Fixing code format"
    VERBATIM
)
