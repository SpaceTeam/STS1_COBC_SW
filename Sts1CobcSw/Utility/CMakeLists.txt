target_sources(Sts1CobcSw_Utility PRIVATE Crc32.cpp)
target_link_libraries(Sts1CobcSw_Utility PUBLIC etl::etl rodos::rodos Sts1CobcSw_Serial)
target_link_libraries(Sts1CobcSw_Utility PRIVATE littlefs::littlefs)
target_compile_definitions(
    Sts1CobcSw_Utility PUBLIC $<$<CONFIG:Debug,RelWithDebInfo>:ENABLE_DEBUG_PRINT>
)
if(ENABLE_DEBUG_PRINT_STACK_USAGE)
    target_compile_definitions(Sts1CobcSw_Utility PUBLIC ENABLE_DEBUG_PRINT_STACK_USAGE)
endif()
