target_sources(Sts1CobcSw_Hal PRIVATE Uart.cpp Spi.cpp)
target_link_libraries(
    Sts1CobcSw_Hal PUBLIC rodos::rodos Sts1CobcSw_Outcome
)
target_compile_definitions(Sts1CobcSw_Hal PUBLIC HW_VERSION=${HW_VERSION})
