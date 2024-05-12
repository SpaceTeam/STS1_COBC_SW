cmake_minimum_required(VERSION 3.22)

include(cmake/prelude.cmake)

project(
    Sts1CobcSw
    VERSION 0.1.0
    DESCRIPTION "Software for the Communication and onboard computer of SpaceTeamSat1"
    HOMEPAGE_URL "https://github.com/SpaceTeam/STS1_COBC_SW"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)
include(cmake/custom-commands.cmake)
include(cmake/clang-tidy-cache.cmake)

# ---- Find dependencies ----

find_package_and_notify(etl)
find_rodos()
find_package_and_notify(littlefs)

# ---- Declare targets ----

add_library(Sts1CobcSw_Dummy STATIC)
add_library(Sts1CobcSw_Edu STATIC)
add_library(Sts1CobcSw_FileSystem STATIC)
add_library(Sts1CobcSw_Outcome INTERFACE)
add_library(Sts1CobcSw_Serial INTERFACE)
add_library(Sts1CobcSw_Utility STATIC)
add_library(Sts1CobcSw_Periphery STATIC)
add_program(HelloDummy)
# add_program(Heartbeat)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    add_library(Sts1CobcSw_Hal STATIC)
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
