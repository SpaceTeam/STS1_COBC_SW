target_sources(Sts1CobcSw_Utility PRIVATE Crc32.cpp Time.cpp)
target_link_libraries(Sts1CobcSw_Utility PUBLIC rodos::rodos type_safe Sts1CobcSw_Serial)
