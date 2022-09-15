# Initialize "global" variable used by add_golden_test()
set(output_files "")

# Add golden tests
# TODO: should also work without golden_tests_folder
add_golden_test(FILE "HelloWorld.test.cpp" LIB rodos::rodos)
add_golden_test(FILE "HelloDummy.test.cpp" LIB rodos::rodos Sts1CobcSw_Dummy)

# Create a single target for all golden tests
add_custom_target(AllGoldenTests DEPENDS ${output_files})
