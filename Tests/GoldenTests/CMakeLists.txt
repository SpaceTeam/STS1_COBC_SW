include(${CMAKE_SOURCE_DIR}/cmake/golden-tests.cmake)

# Initialize "global" variables used by macros
set(output_files "")
set(thread_output_files "")

set(Sts1CobcSw "${CMAKE_SOURCE_DIR}/Sts1CobcSw")

add_golden_test(TESTFILE "HelloWorld.test.cpp" 
					SOURCE "${Sts1CobcSw}/TopicsAndSubscribers.cpp" 
					LIB rodos::rodos)
add_golden_test(TESTFILE "HelloDummy.test.cpp" 
					LIB rodos::rodos Sts1CobcSw_Dummy)

add_custom_target(AllThreadGoldenTests DEPENDS ${thread_output_files})
add_custom_target(AllGoldenTests DEPENDS ${output_files})
