#   message("Hello from thread")
#   
#   add_program(EduProgramQueue)
#   
#   set(Source "${CMAKE_SOURCE_DIR}/Sts1CobcSw")
#   target_sources(Sts1CobcSwTests_EduProgramQueue PRIVATE "${Source}/EduProgramQueueThread.cpp")
#   target_link_libraries(Sts1CobcSwTests_EduProgramQueue PRIVATE Sts1CobcSw_Dummy rodos::rodos)
#   
#   get_property(
#       all_threads_test_targets
#       DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#       PROPERTY BUILDSYSTEM_TARGETS
#   )
#   add_custom_target(AllThreadsTests) # Must be defined after getting all unit test targets
#   add_dependencies(AllThreadsTests ${all_threads_test_targets})

