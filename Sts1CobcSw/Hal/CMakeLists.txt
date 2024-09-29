if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_Hal PRIVATE Spi.cpp Uart.cpp)
    target_link_libraries(Sts1CobcSw_Hal PRIVATE etl::etl)
else()
    target_sources(Sts1CobcSw_Hal PRIVATE SpiMock.cpp)
endif()
target_compile_definitions(Sts1CobcSw_Hal PUBLIC HW_VERSION=${HW_VERSION})
target_link_libraries(
    Sts1CobcSw_Hal PUBLIC rodos::without_main_on_linux Sts1CobcSw_Outcome Sts1CobcSw_Utility
)
target_link_libraries(Sts1CobcSw_Hal PRIVATE strong_type::strong_type)
