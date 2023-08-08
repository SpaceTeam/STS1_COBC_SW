# TODO: Is this not redondant with the top level cmake that does not build libarrie depending on
# target system
if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    add_subdirectory(Periphery)
    add_subdirectory(FileSystem)
    add_subdirectory(Hal)
endif()
add_subdirectory(Edu)
add_subdirectory(Serial)
add_subdirectory(Utility)

target_sources(Sts1CobcSw_Dummy PRIVATE Dummy.cpp)
target_link_libraries(Sts1CobcSw_Dummy PUBLIC etl::etl)

target_sources(Sts1CobcSw_HelloDummy PRIVATE HelloDummy.cpp)
target_link_libraries(Sts1CobcSw_HelloDummy PRIVATE Sts1CobcSw_Dummy rodos::rodos)

# target_sources(Sts1CobcSw_Heartbeat PRIVATE EduHeartbeatThread.cpp TopicsAndSubscribers.cpp)
# target_link_libraries(Sts1CobcSw_Heartbeat PRIVATE etl::etl rodos::rodos type_safe Sts1CobcSw_Hal
# Sts1CobcSw_Utility Sts1CobcSw_Periphery)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    # if(FALSE)
    target_sources(
        Sts1CobcSw_CobcSw
        PRIVATE CommandParser.cpp
                CommandParserThread.cpp
                EduHeartbeatThread.cpp
                EduPowerManagementThread.cpp
                EduProgramQueueThread.cpp
                TopicsAndSubscribers.cpp
                EduCommunicationErrorThread.cpp
                EduListenerThread.cpp
    )
    target_link_libraries(
        Sts1CobcSw_CobcSw
        PRIVATE etl::etl
                rodos::rodos
                type_safe
                Sts1CobcSw_Edu
                Sts1CobcSw_Hal
                Sts1CobcSw_Utility
                Sts1CobcSw_Periphery
    )
endif()
