if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(
        Sts1CobcSw_CobcSoftware
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
        Sts1CobcSw_CobcSoftware
        PRIVATE etl::etl
                rodos::rodos
                Sts1CobcSw_Edu
                Sts1CobcSw_Hal
                Sts1CobcSw_Periphery
                Sts1CobcSw_Serial
                Sts1CobcSw_Utility
    )
endif()
