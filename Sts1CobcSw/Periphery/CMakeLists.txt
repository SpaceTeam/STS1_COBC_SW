target_link_libraries(Sts1CobcSw_Periphery PUBLIC rodos::rodos Sts1CobcSw_Serial)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_Periphery PRIVATE PersistentState.cpp Flash.cpp Fram.cpp Rf.cpp Eps.cpp)
    target_link_libraries(Sts1CobcSw_Periphery PRIVATE Sts1CobcSw_Utility)
    target_link_libraries(Sts1CobcSw_Periphery PUBLIC Sts1CobcSw_Hal)
endif()
