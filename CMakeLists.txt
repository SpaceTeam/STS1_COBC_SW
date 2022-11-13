cmake_minimum_required(VERSION 3.22)

include(cmake/prelude.cmake)

project(
    Sts1CobcSw
    VERSION 0.1.0
    DESCRIPTION "Software for the Communication and onboard computer of Space Team Sat 1"
    HOMEPAGE_URL "https://github.com/SpaceTeam/STS1_COBC_SW"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)
include(cmake/custom-commands.cmake)

# ---- Find dependencies ----

find_package_and_notify(etl)
find_package_and_notify(type_safe)
find_rodos()

# ---- Declare targets ----

add_library(Sts1CobcSw_Dummy OBJECT)
add_library(Sts1CobcSw_Serial INTERFACE)
add_library(Sts1CobcSw_Utility OBJECT)
add_library(Sts1CobcSw_Periphery OBJECT)
add_program(HelloDummy)
#add_program(Heartbeat)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    add_library(Sts1CobcSw_Hal INTERFACE)
    add_program(CobcSw)
endif()

# All targets must only add the top level project directory as PUBLIC include directory
all_targets_include_directories("$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>")

add_subdirectory(${PROJECT_NAME})

# ---- Developer mode ----

if(NOT Sts1CobcSw_DEVELOPER_MODE)
    return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
    message(AUTHOR_WARNING "Developer mode is intended for developers of CobcSw")
endif()

include(cmake/dev-mode.cmake)
