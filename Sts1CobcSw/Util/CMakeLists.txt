target_sources(Sts1CobcSw_Util PRIVATE Util.cpp Time.cpp)
target_link_libraries(Sts1CobcSw_Util PRIVATE rodos::rodos etl::etl)
