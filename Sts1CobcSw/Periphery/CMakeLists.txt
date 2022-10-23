target_sources(Sts1CobcSw_Periphery PRIVATE Edu.cpp)
target_link_libraries(Sts1CobcSw_Periphery PUBLIC rodos::rodos etl::etl Sts1CobcSw_Hal Sts1CobcSw_Utility Sts1CobcSw_Serial)