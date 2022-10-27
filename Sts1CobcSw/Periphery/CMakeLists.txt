target_sources(Sts1CobcSw_Periphery PRIVATE PersistentState.cpp)
target_link_libraries(Sts1CobcSw_Periphery PUBLIC type_safe)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_Periphery PRIVATE Flash.cpp)
    target_link_libraries(
        Sts1CobcSw_Periphery PRIVATE rodos::rodos Sts1CobcSw_Serial Sts1CobcSw_Hal
    )
endif()
