add_program(EduPowerManagement)
target_sources(
    Sts1CobcSwTests_EduPowerManagement
    PRIVATE EduPowerManagement.test.cpp
            "${CMAKE_SOURCE_DIR}/Sts1CobcSw/CobcSoftware/EduPowerManagementThread.cpp"
            "${CMAKE_SOURCE_DIR}/Sts1CobcSw/CobcSoftware/EduHeartbeatThread.cpp"
            "${CMAKE_SOURCE_DIR}/Sts1CobcSw/CobcSoftware/TopicsAndSubscribers.cpp"
)
target_link_libraries(
    Sts1CobcSwTests_EduPowerManagement PRIVATE rodos::rodos Sts1CobcSw_Edu Sts1CobcSw_Hal
                                               Sts1CobcSwTests::HardwareSetup
)

get_property(thread_test_targets DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BUILDSYSTEM_TARGETS)
add_custom_target(AllThreadTests) # Must be defined after getting all edu test targets
add_dependencies(AllThreadTests ${thread_test_targets})
