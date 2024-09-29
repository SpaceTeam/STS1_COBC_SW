target_sources(Sts1CobcSw_Hal PRIVATE Uart.cpp)
target_link_libraries(
    Sts1CobcSw_Hal PUBLIC etl::etl rodos::without_main_on_linux Sts1CobcSw_Utility
                          Sts1CobcSw_Outcome
)
target_compile_definitions(Sts1CobcSw_Hal PUBLIC HW_VERSION=${HW_VERSION})

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_Hal PRIVATE Spi.cpp)
else()
    target_sources(Sts1CobcSw_Hal PRIVATE SpiMock.cpp)
endif()
