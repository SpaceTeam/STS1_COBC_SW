cmake_minimum_required(VERSION 3.22)

include(cmake/prelude.cmake)

project(
    CobcSw
    VERSION 0.1.0
    DESCRIPTION "Communication and onboard computer software for Space Team Sat 1"
    HOMEPAGE_URL "https://github.com/SpaceTeam/STS1_COBC_SW"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)
include(cmake/custom-commands.cmake)

# ---- Find dependencies ----

find_package_and_notify(etl)
find_rodos()

# ---- Declare targets ----

add_library(CobcSw_Lib OBJECT)
add_program(HelloWorld)

add_subdirectory(Source)

# ---- Developer mode ----

if(NOT CobcSw_DEVELOPER_MODE)
    return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
    message(AUTHOR_WARNING "Developer mode is intended for developers of CobcSw")
endif()

include(cmake/dev-mode.cmake)
