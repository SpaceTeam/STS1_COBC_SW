if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(
        Sts1CobcSw_CobcSw
        PRIVATE CommandParserThread.cpp
                EduHeartbeatThread.cpp
                EduListenerThread.cpp
                EduPowerManagementThread.cpp
                EduProgramQueueThread.cpp
                FlashStartupTestThread.cpp
                FramEpsStartupTestThread.cpp
                RfStartupTestThread.cpp
                SpiStartupTestAndSupervisorThread.cpp
                CommandParser.cpp
                TopicsAndSubscribers.cpp
    )
    target_link_libraries(
        Sts1CobcSw_CobcSw
        PRIVATE etl::etl
                rodos::rodos
                Sts1CobcSw_Edu
                Sts1CobcSw_Hal
                Sts1CobcSw_Periphery
                Sts1CobcSw_Serial
                Sts1CobcSw_Utility
    )
endif()
