# ---- Developer mode ----

# Developer mode enables targets and code paths in the CMake scripts that are only relevant for the
# developer(s) of CobcSw Targets necessary to build the project must be provided unconditionally, so
# consumers can trivially build and package the project
if(PROJECT_IS_TOP_LEVEL)
    option(Sts1CobcSw_DEVELOPER_MODE "Enable developer mode" OFF)
endif()

# ---- Warning guard ----

# target_include_directories with the SYSTEM modifier will request the compiler to omit warnings
# from the provided paths, if the compiler supports that This is to provide a user experience
# similar to find_package when add_subdirectory or FetchContent is used to consume this project
set(warning_guard "")
if(NOT PROJECT_IS_TOP_LEVEL)
    option(CobcSw_INCLUDES_WITH_SYSTEM
           "Use SYSTEM modifier for CobcSw's includes, disabling warnings" ON
    )
    mark_as_advanced(CobcSw_INCLUDES_WITH_SYSTEM)
    if(CobcSw_INCLUDES_WITH_SYSTEM)
        set(warning_guard SYSTEM)
    endif()
endif()

# ---- Other variables ----

set(HW_VERSION 30 CACHE STRING "Hardware version")

if(CMAKE_BUILD_TYPE STREQUAL "Sanitize")
    set(EXTRA_SANITIZER_STACK_SIZE 20000)
else()
    set(EXTRA_SANITIZER_STACK_SIZE 0)
endif()
message("Adding -DEXTRA_SANITIZER_STACK_SIZE=${EXTRA_SANITIZER_STACK_SIZE} to all targets")
add_compile_definitions(EXTRA_SANITIZER_STACK_SIZE=${EXTRA_SANITIZER_STACK_SIZE})
