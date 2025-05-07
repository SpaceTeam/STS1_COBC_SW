target_sources(Sts1CobcSw_Utility PRIVATE Crc32.cpp)
target_link_libraries(
    Sts1CobcSw_Utility PUBLIC etl::etl rodos::without_main_on_linux Sts1CobcSw_Serial
)
target_link_libraries(Sts1CobcSw_Utility PRIVATE littlefs::littlefs)
target_compile_definitions(
    Sts1CobcSw_Utility PUBLIC $<$<CONFIG:Debug,RelWithDebInfo>:ENABLE_DEBUG_PRINT>
)
