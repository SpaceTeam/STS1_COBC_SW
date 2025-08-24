target_link_libraries(Sts1CobcSw_Utility INTERFACE etl::etl rodos::rodos Sts1CobcSw_Serial)
target_compile_definitions(
    Sts1CobcSw_Utility INTERFACE $<$<CONFIG:Debug,RelWithDebInfo>:ENABLE_DEBUG_PRINT>
)
if(ENABLE_DEBUG_PRINT_STACK_USAGE)
    target_compile_definitions(Sts1CobcSw_Utility INTERFACE ENABLE_DEBUG_PRINT_STACK_USAGE)
endif()
