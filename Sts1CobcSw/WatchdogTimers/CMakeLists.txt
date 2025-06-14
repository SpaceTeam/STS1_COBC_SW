if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_WatchdogTimers PRIVATE WatchdogTimers.cpp)
    target_link_libraries(
        Sts1CobcSw_WatchdogTimers PRIVATE rodos::rodos strong_type::strong_type Sts1CobcSw_Hal
                                          Sts1CobcSw_Vocabulary
    )
endif()
