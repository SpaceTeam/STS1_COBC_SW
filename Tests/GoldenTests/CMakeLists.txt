include(${CMAKE_SOURCE_DIR}/cmake/golden-tests.cmake)

# Initialize "global" variable used by add_golden_test()
set(output_files "")

# Add golden tests
add_golden_test(FILE "HelloWorld.test.cpp" LIB rodos::rodos)
add_golden_test(FILE "HelloDummy.test.cpp" LIB rodos::rodos Sts1CobcSw_Dummy)
# Problem : we need CommandParser.cpp
#add_golden_test(FILE "Dispatch.test.cpp" LIB rodos::rodos 
#Sts1CobcSw_Utility 
#Sts1CobcSw_Serial 
#type_safe)

# Initialize "global" variable used by add_thread_golden_test()
set(thread_output_files "")
set(Source "${CMAKE_SOURCE_DIR}/Sts1CobcSw")
#add_thread_golden_test(FILES "${Source}/EduProgramQueueThread.cpp" "${Source}/TopicsAndSubscribers.cpp" "${Source}/EduProgramQueue.cpp" LIB rodos::rodos Sts1CobcSw_Periphery Sts1CobcSw_Utility )
add_thread_golden_test(FILES "Dispatch.test.cpp" 
							 "${Source}/CommandParser.cpp" 
							 "${Source}/EduProgramQueue.cpp" 
					   LIB rodos::rodos 
					   	   Sts1CobcSw_Serial 
						   Sts1CobcSw_Utility)
add_custom_target(AllThreadGoldenTests DEPENDS ${thread_output_files})

# Create a single target for all golden tests
add_custom_target(AllGoldenTests DEPENDS ${output_files})
