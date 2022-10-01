target_sources(Sts1CobcSw_Periphery PRIVATE Edu.cpp)
target_link_libraries(Sts1CobcSw_Periphery PRIVATE rodos::rodos etl::etl Sts1CobcSw_Hal Sts1CobcSw_Util)