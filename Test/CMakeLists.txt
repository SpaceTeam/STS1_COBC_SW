# Parent project does not export its library target, so this CML implicitly depends on being added
# from it, i.e. the testing is done only from the build tree and is not feasible from an install
# location

# Only include tests when building for Linux
if(NOT (CMAKE_SYSTEM_NAME STREQUAL Linux))
    return()
endif()

project(CobcSwTests LANGUAGES CXX)

include(${CMAKE_SOURCE_DIR}/cmake/custom-commands.cmake)

# ---- Find libraries ----

find_package_and_notify(Catch2)
find_package_and_notify(etl)
find_package_and_notify(type_safe)
find_rodos()


# ---- Tests ----

# Initialize variables
set(output_files "")

# Add golden tests
set(golden_tests_folder "${CMAKE_CURRENT_SOURCE_DIR}/Source/GoldenTests")
add_golden_test(FILE "${golden_tests_folder}/HelloWorld.cpp" LIB rodos::rodos)
add_golden_test(FILE "${golden_tests_folder}/HelloCobc.cpp" LIB rodos::rodos CobcSw_Lib)

# Create a single target for all BlackBox tests.
add_custom_target(BlackBoxTests DEPENDS "${output_files}")

add_executable(CobcSw_Test EXCLUDE_FROM_ALL)


# ---- Main test target ----
add_custom_target(Tests)
add_dependencies(Tests BlackBoxTests CobcSw_Test)

add_subdirectory(Source)

include(Catch)
catch_discover_tests(CobcSw_Test)


# ---- End-of-file commands ----

add_folders(Tests)
