cmake_minimum_required(VERSION 3.31)

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

find_package_and_notify(etl)
find_package_and_notify(littlefs)
find_package_and_notify(strong_type)

add_library(Sts1CobcSw_ErrorDetectionAndCorrection STATIC)
add_library(Sts1CobcSw_Serial INTERFACE)
add_library(Sts1CobcSw_Utility INTERFACE)
add_library(Sts1CobcSw_Vocabulary STATIC)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    add_library(Sts1CobcSw_CmsisDevice INTERFACE)
endif()

if(BUILD_BOOTLOADER)
    add_program(Bootloader)
else()
    find_package_and_notify(libfec)
    find_rodos()
    if(BUILD_FOR_USE_WITH_BOOTLOADER)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
    endif()

    add_library(Sts1CobcSw_Blake2s STATIC)
    add_library(Sts1CobcSw_ChannelCoding STATIC)
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
    add_library(Sts1CobcSw_Mailbox INTERFACE)
    add_library(Sts1CobcSw_Telemetry STATIC)
    if(CMAKE_SYSTEM_NAME STREQUAL Generic)
        add_library(Sts1CobcSw_WatchdogTimers STATIC)
        add_program(Firmware)
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
