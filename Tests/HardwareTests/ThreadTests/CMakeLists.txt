set(source_directory ${CMAKE_SOURCE_DIR}/Sts1CobcSw)

add_program(EduPowerManagement)
target_sources(
    Sts1CobcSwTests_EduPowerManagement
    PRIVATE EduPowerManagement.test.cpp
            "${source_directory}/CobcSoftware/EduPowerManagementThread.cpp"
            "${source_directory}/CobcSoftware/EduHeartbeatThread.cpp"
            "${source_directory}/CobcSoftware/TopicsAndSubscribers.cpp"
)
target_link_libraries(
    Sts1CobcSwTests_EduPowerManagement PRIVATE rodos::rodos Sts1CobcSw_Hal Sts1CobcSw_Edu
                                               Sts1CobcSwTests_RfLatchupDisablePin
)

get_property(thread_test_targets DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BUILDSYSTEM_TARGETS)

# Add the top level project directory as include directory for all thread tests
foreach(target IN LISTS thread_test_targets)
    target_include_directories(${target} PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>)
endforeach()

add_custom_target(AllThreadTests) # Must be defined after getting all edu test targets
add_dependencies(AllThreadTests ${thread_test_targets})
