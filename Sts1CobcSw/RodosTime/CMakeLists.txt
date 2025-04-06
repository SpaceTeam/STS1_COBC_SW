target_sources(Sts1CobcSw_RodosTime PRIVATE RodosTime.cpp)
target_link_libraries(
    Sts1CobcSw_RodosTime PUBLIC rodos::without_main_on_linux strong_type::strong_type
                                Sts1CobcSw_Outcome Sts1CobcSw_Vocabulary
)
