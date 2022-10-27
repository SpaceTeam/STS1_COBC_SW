target_sources(Sts1CobcSw_Periphery PRIVATE Edu.cpp EduStructs.cpp Flash.cpp PersistentState.cpp)
target_link_libraries(
    Sts1CobcSw_Periphery PUBLIC rodos::rodos type_safe Sts1CobcSw_Hal Sts1CobcSw_Serial
)
target_link_libraries(Sts1CobcSw_Periphery PRIVATE Sts1CobcSw_Utility)
