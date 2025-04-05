target_link_libraries(Sts1CobcSw_Sensors PUBLIC Sts1CobcSw_Serial)
if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_Sensors PRIVATE Eps.cpp TemperatureSensor.cpp)
    target_link_libraries(
        Sts1CobcSw_Sensors
        PRIVATE rodos::rodos
                strong_type::strong_type
                Sts1CobcSw_FramSections
                Sts1CobcSw_Hal
                Sts1CobcSw_RodosTime
                Sts1CobcSw_Utility
                Sts1CobcSw_Vocabulary
    )
else()
    target_sources(Sts1CobcSw_Sensors PRIVATE EpsStubs.cpp TemperatureSensorStubs.cpp)
endif()
