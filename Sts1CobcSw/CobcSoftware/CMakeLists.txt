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
                RfCommunicationThread.cpp
                RfStartupTestThread.cpp
                SpiStartupTestAndSupervisorThread.cpp
                TelemetryThread.cpp
                TopicsAndSubscribers.cpp
    )
    target_link_libraries(
        Sts1CobcSw_CobcSoftware
        PRIVATE etl::etl
                rodos::rodos
                strong_type::strong_type
                Sts1CobcSw_Edu
                Sts1CobcSw_FramSections
                Sts1CobcSw_Hal
                Sts1CobcSw_Outcome
                Sts1CobcSw_Periphery
                Sts1CobcSw_ProgramId
                Sts1CobcSw_Serial
                Sts1CobcSw_Telemetry
                Sts1CobcSw_Utility
    )
endif()
