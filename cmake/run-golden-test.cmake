cmake_minimum_required(VERSION 3.22)

if(NOT DEFINED TEST_EXECUTABLE)
    message(FATAL_ERROR "TEST_EXECUTABLE is not defined")
endif()
if(NOT DEFINED EXPECTED_OUTPUT_FILE)
    message(FATAL_ERROR "EXPECTED_OUTPUT_FILE is not defined")
endif()
if(NOT DEFINED BUILD_CONFIG)
    message(FATAL_ERROR "BUILD_CONFIG is not defined")
endif()

# Check if there is a config specific expected output file
cmake_path(GET EXPECTED_OUTPUT_FILE STEM file_stem)
cmake_path(GET EXPECTED_OUTPUT_FILE PARENT_PATH directory)
set(config_specific_expected_output_file "${directory}/${file_stem}_${BUILD_CONFIG}.txt")
if(EXISTS "${config_specific_expected_output_file}")
    set(EXPECTED_OUTPUT_FILE "${config_specific_expected_output_file}")
endif()

message("Running golden test with")
message("  test executable:      ${TEST_EXECUTABLE}")
message("  expected output file: ${EXPECTED_OUTPUT_FILE}")
message("  build config:         ${BUILD_CONFIG}")
execute_process(COMMAND "${TEST_EXECUTABLE}" OUTPUT_VARIABLE output)

set(rodos_header_regex ".*--------------- Application running ------------\n")
string(REGEX REPLACE "${rodos_header_regex}" "" output_without_rodos_header ${output})

file(READ "${EXPECTED_OUTPUT_FILE}" expected_output)

if(output_without_rodos_header STREQUAL expected_output)
    message("Test passed ✔️")
else()
    # TODO: Upgrade CMake to version 3.29 for cmake_language(EXIT 1)
    message(FATAL_ERROR "Test failed ❌")
endif()
