if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    add_subdirectory(Hal)
endif()
add_subdirectory(Periphery)
add_subdirectory(Edu)
add_subdirectory(FileSystem)
add_subdirectory(Outcome)
add_subdirectory(Serial)
add_subdirectory(Utility)

target_sources(Sts1CobcSw_Dummy PRIVATE Dummy.cpp)
target_link_libraries(Sts1CobcSw_Dummy PUBLIC etl::etl Sts1CobcSw_Utility)

target_sources(Sts1CobcSw_HelloDummy PRIVATE HelloDummy.cpp)
target_link_libraries(Sts1CobcSw_HelloDummy PRIVATE Sts1CobcSw_Dummy rodos::rodos)

# target_sources(Sts1CobcSw_Heartbeat PRIVATE EduHeartbeatThread.cpp TopicsAndSubscribers.cpp)
# target_link_libraries(Sts1CobcSw_Heartbeat PRIVATE etl::etl rodos::rodos Sts1CobcSw_Hal
# Sts1CobcSw_Utility Sts1CobcSw_Periphery)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(
        Sts1CobcSw_CobcSw
        PRIVATE CommandParser.cpp
                CommandParserThread.cpp
                EduCommunicationErrorThread.cpp
                EduHeartbeatThread.cpp
                EduListenerThread.cpp
                EduPowerManagementThread.cpp
                EduProgramQueueThread.cpp
                FlashStartupTestThread.cpp
                FramEpsStartupTestThread.cpp
                RfStartupTestThread.cpp
                SpiStartupTestAndSupervisorThread.cpp
                TopicsAndSubscribers.cpp
    )
    target_link_libraries(
        Sts1CobcSw_CobcSw PRIVATE etl::etl rodos::rodos Sts1CobcSw_Edu Sts1CobcSw_Hal
                                  Sts1CobcSw_Periphery Sts1CobcSw_Serial Sts1CobcSw_Utility
    )
endif()
