target_sources(Sts1CobcSw_Hal PRIVATE Uart.cpp Spi.cpp)
target_link_libraries(
    Sts1CobcSw_Hal PUBLIC etl::etl rodos::rodos Sts1CobcSw_Utility Sts1CobcSw_Outcome
)
