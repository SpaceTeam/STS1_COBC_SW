target_sources(Sts1CobcSw_RfProtocols PRIVATE TmTransferFrame.cpp)
target_link_libraries(
    Sts1CobcSw_RfProtocols PUBLIC etl::etl rodos::without_main_on_linux Sts1CobcSw_Outcome
                                  Sts1CobcSw_Serial
)
