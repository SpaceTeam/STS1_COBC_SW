cmake_minimum_required(VERSION 3.22)

include(cmake/prelude.cmake)

project(
    Sts1CobcSw
    VERSION 0.1.0
    DESCRIPTION "Software for the communication and onboard computer of SpaceTeamSat1"
    HOMEPAGE_URL "https://github.com/SpaceTeam/STS1_COBC_SW"
    LANGUAGES ASM CXX
)

include(cmake/options-and-variables.cmake)
include(cmake/ccache.cmake)
include(cmake/clang-tidy-cache.cmake)
include(cmake/custom-commands.cmake)

if(BUILD_BOOTLOADER)
    add_program(Bootloader)
else()
    find_package_and_notify(etl)
    find_rodos()
    find_package_and_notify(littlefs)
    find_package_and_notify(strong_type)

    add_library(Sts1CobcSw_Blake2s STATIC)
    add_library(Sts1CobcSw_Edu STATIC)
    add_library(Sts1CobcSw_FileSystem STATIC)
    add_library(Sts1CobcSw_FirmwareManagement STATIC)
    add_library(Sts1CobcSw_Flash STATIC)
    add_library(Sts1CobcSw_Fram STATIC)
    add_library(Sts1CobcSw_FramSections INTERFACE)
    add_library(Sts1CobcSw_Hal STATIC)
    add_library(Sts1CobcSw_Outcome INTERFACE)
    add_library(Sts1CobcSw_RealTime STATIC)
    add_library(Sts1CobcSw_Rf STATIC)
    add_library(Sts1CobcSw_RfProtocols STATIC)
    add_library(Sts1CobcSw_RodosTime STATIC)
    add_library(Sts1CobcSw_Sensors STATIC)
    add_library(Sts1CobcSw_Serial INTERFACE)
    add_library(Sts1CobcSw_SingleBuffer INTERFACE)
    add_library(Sts1CobcSw_Telemetry STATIC)
    add_library(Sts1CobcSw_Utility STATIC)
    add_library(Sts1CobcSw_Vocabulary INTERFACE)

    if(CMAKE_SYSTEM_NAME STREQUAL Generic)
        # TODO: Rename to (Cobc)Firmware
        add_program(CobcSoftware)
    endif()
endif()

# All targets must only add the top level project directory as PUBLIC include directory
all_targets_include_directories("$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>")

add_subdirectory(${PROJECT_NAME})

# ---- Developer mode ----

if(NOT Sts1CobcSw_DEVELOPER_MODE)
    return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
    message(AUTHOR_WARNING "Developer mode is intended for developers of Sts1CobcSw")
endif()

include(cmake/dev-mode.cmake)
