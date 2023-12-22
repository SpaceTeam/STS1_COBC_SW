target_sources(Sts1CobcSw_Hal PRIVATE Communication.cpp)
target_link_libraries(
    Sts1CobcSw_Hal PUBLIC etl::etl rodos::rodos Sts1CobcSw_Serial Sts1CobcSw_Utility
)
