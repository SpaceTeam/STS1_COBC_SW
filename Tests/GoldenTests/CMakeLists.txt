include(${CMAKE_SOURCE_DIR}/cmake/golden-tests.cmake)

# Initialize "global" variable used by add_golden_test()
set(output_files "")

# Add golden tests
add_golden_test(FILE "HelloWorld.test.cpp" LIB rodos::rodos)
add_golden_test(FILE "HelloDummy.test.cpp" LIB rodos::rodos Sts1CobcSw_Dummy)
# TODO: Fix the two tests below
# add_golden_test(FILE "EduQueue.test.cpp" LIB rodos::rodos Sts1CobcSw_Dummy)
# add_golden_test(FILE "BuildQueueCommand.test.cpp" LIB rodos::rodos Sts1CobcSw_Dummy etl::etl Sts1CobcSw_Util)

# Create a single target for all golden tests
add_custom_target(AllGoldenTests DEPENDS ${output_files})
