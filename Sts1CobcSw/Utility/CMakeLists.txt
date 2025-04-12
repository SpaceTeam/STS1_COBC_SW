target_sources(Sts1CobcSw_Utility PRIVATE Crc32.cpp)
target_link_libraries(
    Sts1CobcSw_Utility PUBLIC etl::etl littlefs::littlefs rodos::without_main_on_linux
                              strong_type::strong_type Sts1CobcSw_Serial Sts1CobcSw_Vocabulary
)
target_compile_definitions(
    Sts1CobcSw_Utility PUBLIC $<$<CONFIG:Debug,RelWithDebInfo>:ENABLE_DEBUG_PRINT>
)
