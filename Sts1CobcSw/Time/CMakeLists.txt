target_link_libraries(
    Sts1CobcSw_Time INTERFACE rodos::without_main_on_linux strong_type::strong_type
                              Sts1CobcSw_FramSections Sts1CobcSw_Vocabulary
)
