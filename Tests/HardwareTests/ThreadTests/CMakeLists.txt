set(source_directory ${CMAKE_SOURCE_DIR}/Sts1CobcSw)

add_program(EduPowerManagement)
target_sources(
    Sts1CobcSwTests_EduPowerManagement
    PRIVATE EduPowerManagement.test.cpp ${source_directory}/EduPowerManagementThread.cpp ${source_directory}/EduHeartbeatThread.cpp
            ${source_directory}/TopicsAndSubscribers.cpp
)
target_link_libraries(
    Sts1CobcSwTests_EduPowerManagement PRIVATE rodos::rodos type_safe Sts1CobcSw_Periphery
                                               Sts1CobcSw_Hal Sts1CobcSw_Utility
)

add_program(CommunicationErrorBusyWaiting)
target_sources(
    Sts1CobcSwTests_CommunicationErrorBusyWaiting 
    PRIVATE 
    CommunicationErrorBusyWaiting.test.cpp 
    ${source_directory}/EduCommunicationErrorThread.cpp ${source_directory}/EduCommunicationErrorThread.hpp
    ${source_directory}/EduHeartbeatThread.cpp
    ${source_directory}/TopicsAndSubscribers.cpp
    ${source_directory}/EduProgramQueueThread.cpp ${source_directory}/EduProgramQueueThread.hpp
    ${source_directory}/EduProgramQueue.cpp ${source_directory}/EduProgramQueue.hpp
    )
target_link_libraries(Sts1CobcSwTests_CommunicationErrorBusyWaiting PRIVATE rodos::rodos type_safe Sts1CobcSw_Utility Sts1CobcSw_Periphery)

get_property(
    thread_test_targets
    DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    PROPERTY BUILDSYSTEM_TARGETS
)

# Add the top level project directory as include directory for all thread tests
foreach(target IN LISTS thread_test_targets)
    target_include_directories(${target} PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>)
endforeach()

add_custom_target(AllThreadTests) # Must be defined after getting all edu test targets
add_dependencies(AllThreadTests ${thread_test_targets})
