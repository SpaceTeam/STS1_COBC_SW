target_sources(Sts1CobcSw_RealTime PRIVATE RealTime.cpp)
target_link_libraries(
    Sts1CobcSw_RealTime PUBLIC rodos::without_main_on_linux strong_type::strong_type
                               Sts1CobcSw_FramSections Sts1CobcSw_RodosTime Sts1CobcSw_Vocabulary
)
