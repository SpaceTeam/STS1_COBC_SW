target_link_libraries(
    Sts1CobcSw_RfProtocols INTERFACE etl::etl rodos::without_main_on_linux Sts1CobcSw_Outcome
                                     Sts1CobcSw_Serial
)
