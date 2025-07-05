target_link_libraries(
    Sts1CobcSw_Rf PRIVATE Sts1CobcSw_Outcome Sts1CobcSw_Serial Sts1CobcSw_Vocabulary
)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    if(REDIRECT_RF_OVER_UCI)
        target_sources(Sts1CobcSw_Rf PRIVATE RfOverUci.cpp)
        target_link_libraries(
            Sts1CobcSw_Rf
            PRIVATE rodos::rodos
                    strong_type::strong_type
                    Sts1CobcSw_ChannelCoding
                    Sts1CobcSw_FramSections
                    Sts1CobcSw_Hal
                    Sts1CobcSw_Outcome
                    Sts1CobcSw_Utility
        )
    else()
        target_sources(Sts1CobcSw_Rf PRIVATE Rf.cpp)
        target_link_libraries(
            Sts1CobcSw_Rf
            PRIVATE rodos::rodos
                    strong_type::strong_type
                    Sts1CobcSw_ChannelCoding
                    Sts1CobcSw_FramSections
                    Sts1CobcSw_Hal
                    Sts1CobcSw_RodosTime
                    Sts1CobcSw_Utility
                    Sts1CobcSw_WatchdogTimers
        )
    endif()
else()
    target_sources(Sts1CobcSw_Rf PRIVATE RfMock.cpp)
endif()
