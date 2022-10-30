# TODO: Break cyclic dependency with Sts1CobcSw_Utility
target_link_libraries(Sts1CobcSw_Serial INTERFACE type_safe Sts1CobcSw_Utility)
