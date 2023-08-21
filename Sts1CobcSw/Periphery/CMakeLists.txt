target_link_libraries(Sts1CobcSw_Periphery PUBLIC rodos::rodos type_safe Sts1CobcSw_Serial)
target_link_libraries(Sts1CobcSw_Periphery PRIVATE Sts1CobcSw_Utility)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_Periphery PRIVATE PersistentState.cpp Flash.cpp Fram.cpp)
    target_link_libraries(Sts1CobcSw_Periphery PUBLIC Sts1CobcSw_Hal)
endif()
