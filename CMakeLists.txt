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

# ---- Declare library ----

add_library(CobcSw_Lib OBJECT Source/Lib.cpp)

target_include_directories(
    CobcSw_Lib ${warning_guard} PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Source>"
)

target_compile_features(CobcSw_Lib PUBLIC cxx_std_20)

# ---- Declare executable ----

add_executable(CobcSw_HelloWorld Source/HelloWorld.cpp)
add_executable(CobcSw::exe ALIAS CobcSw_HelloWorld)

set_target_properties(CobcSw_HelloWorld PROPERTIES OUTPUT_NAME HelloWorld EXPORT_NAME HelloWorld)

target_compile_features(CobcSw_HelloWorld PRIVATE cxx_std_20)

find_rodos()

target_link_libraries(CobcSw_HelloWorld PRIVATE CobcSw_Lib rodos::rodos)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
    include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT CobcSw_DEVELOPER_MODE)
    return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
    message(AUTHOR_WARNING "Developer mode is intended for developers of CobcSw")
endif()

include(cmake/dev-mode.cmake)
