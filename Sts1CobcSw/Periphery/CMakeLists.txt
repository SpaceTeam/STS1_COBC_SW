target_link_libraries(
    Sts1CobcSw_Periphery
    PUBLIC rodos::without_main_on_linux strong_type::strong_type Sts1CobcSw_Hal Sts1CobcSw_Outcome
           Sts1CobcSw_Serial Sts1CobcSw_Utility
)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(
        Sts1CobcSw_Periphery PRIVATE Eps.cpp Flash.cpp Fram.cpp Rf.cpp Spis.cpp
                                     TemperatureSensor.cpp
    )
else()
    target_sources(
        Sts1CobcSw_Periphery PRIVATE EpsStubs.cpp FlashMock.cpp FramMock.cpp SpiMocks.cpp
                                     TemperatureSensorStubs.cpp
    )
endif()
