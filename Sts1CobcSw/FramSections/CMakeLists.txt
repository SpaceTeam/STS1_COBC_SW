target_link_libraries(
    Sts1CobcSw_FramSections INTERFACE rodos::without_main_on_linux Sts1CobcSw_Periphery
                                      Sts1CobcSw_Serial Sts1CobcSw_Utility
)
