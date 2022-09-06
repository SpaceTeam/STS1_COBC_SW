# Parent project does not export its library target, so this CML implicitly depends on being added
# from it, i.e. the testing is done only from the build tree and is not feasible from an install
# location

# Only include tests when building for Linux
if(NOT (CMAKE_SYSTEM_NAME STREQUAL Linux))
    return()
endif()

project(CobcSwTests LANGUAGES CXX)


# ---- Find libraries ----

find_package_and_notify(Catch2)
find_package_and_notify(etl)
find_package_and_notify(type_safe)
find_rodos()


# ---- Tests ----

file(GLOB test_files
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    Source/BlackBox/*.cpp)

foreach(test_file ${test_files})

    get_filename_component(filename ${test_file} NAME_WE)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" test_name ${test_file})

    # Compile rodos test application
    add_executable("${test_name}-bin" EXCLUDE_FROM_ALL ${test_file})
    target_link_libraries("${test_name}-bin" PUBLIC rodos::rodos CobcSw_Lib)

    # Command to run rodos test application, generating output files.
    add_custom_command(
        OUTPUT
        "${test_name}-bin.output"
        COMMAND
        bash
        ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/test-runner.sh
        $<TARGET_FILE:${test_name}-bin>
        ${CMAKE_CURRENT_SOURCE_DIR}/expected-outputs/${filename}.txt
        DEPENDS
        ${test_name}-bin Scripts/test-runner.sh)

    list(APPEND output_files "${test_name}-bin.output")

    add_custom_target(${test_name}-clean
        COMMAND
        ${CMAKE_COMMAND} -E remove -f
        "${test_name}-bin.output")

    add_test(
        NAME
        ${test_name}-test
        COMMAND
        diff ${test_name}-bin.output ${CMAKE_CURRENT_SOURCE_DIR}/ExpectedOutputs/${filename}.txt
        )

endforeach()

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
