# ---- Options and cache variables ----

option(BUILD_BOOTLOADER "Build the bootloader instead of the firmware" OFF)
if(BUILD_BOOTLOADER AND CMAKE_SYSTEM_NAME STREQUAL Linux)
    message(FATAL_ERROR "Building the bootloader is only supported on embedded platforms")
endif()

set(HW_VERSION 30 CACHE STRING "Hardware version")

# Developer mode enables targets and code paths in the CMake scripts that are only relevant for the
# developer(s) of STS1 COBC SW. Targets necessary to build the project must be provided
# unconditionally, so consumers can trivially build and package the project.
if(PROJECT_IS_TOP_LEVEL)
    option(Sts1CobcSw_DEVELOPER_MODE "Enable developer mode" OFF)
endif()

# ---- Normal (non-cache) variables ----

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    if(BUILD_BOOTLOADER)
        set(linker_script "${CMAKE_SOURCE_DIR}/Scripts/Bootloader.ld")
    else()
        set(linker_script "${CMAKE_SOURCE_DIR}/Scripts/Stm32f411xe.ld")
    endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Sanitize")
    set(extra_sanitize_stack_size 20000)
else()
    set(extra_sanitize_stack_size 0)
endif()
message("Adding -DEXTRA_SANITIZER_STACK_SIZE=${extra_sanitize_stack_size} to all targets")
add_compile_definitions(EXTRA_SANITIZER_STACK_SIZE=${extra_sanitize_stack_size})
