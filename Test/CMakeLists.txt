# Parent project does not export its library target, so this CML implicitly depends on being added
# from it, i.e. the testing is done only from the build tree and is not feasible from an install
# location

# Only include tests when building for Linux
if(NOT (CMAKE_SYSTEM_NAME STREQUAL Linux))
    return()
endif()

project(Sts1CobcSwTests LANGUAGES CXX)

# ---- Find libraries ----

find_package_and_notify(Catch2)
find_package_and_notify(etl)
find_package_and_notify(type_safe)
find_rodos()

# ---- Tests ----

# # Initialize variables
# set(output_files "")

# # Add golden tests
# set(golden_tests_folder "${CMAKE_CURRENT_SOURCE_DIR}/GoldenTests")
# add_golden_test(FILE "${golden_tests_folder}/HelloWorld.cpp" LIB rodos::rodos)
# add_golden_test(FILE "${golden_tests_folder}/HelloCobc.cpp" LIB rodos::rodos Sts1CobcSw_Dummy)

# # Create a single target for all golden tests
# add_custom_target(GoldenTests DEPENDS "${output_files}")

# Add unit tests
add_executable(Sts1CobcSwTests_Dummy EXCLUDE_FROM_ALL)

# # Add a custom target that depends on all tests
# add_custom_target(Tests)
# add_dependencies(Tests GoldenTests Sts1CobcSwTests_Dummy)

add_subdirectory(UnitTests)

include(Catch)
catch_discover_tests(Sts1CobcSwTests_Dummy)

# ---- End-of-file commands ----

add_folders(Test)
