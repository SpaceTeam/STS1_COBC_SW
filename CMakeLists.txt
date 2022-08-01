cmake_minimum_required(VERSION 3.14)

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

# ---- Declare library ----

add_library(
    CobcSw_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    CobcSw_lib ${warning_guard}
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(CobcSw_lib PUBLIC cxx_std_20)

# ---- Declare executable ----

add_executable(CobcSw_exe source/main.cpp)
add_executable(CobcSw::exe ALIAS CobcSw_exe)

set_target_properties(
    CobcSw_exe PROPERTIES
    OUTPUT_NAME CobcSw
    EXPORT_NAME exe
)

target_compile_features(CobcSw_exe PRIVATE cxx_std_20)

target_link_libraries(CobcSw_exe PRIVATE CobcSw_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT CobcSw_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of CobcSw"
  )
endif()

include(cmake/dev-mode.cmake)
