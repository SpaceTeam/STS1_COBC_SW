target_link_libraries(
    Sts1CobcSw_Fram PUBLIC strong_type::strong_type Sts1CobcSw_Serial Sts1CobcSw_Utility
                           Sts1CobcSw_Vocabulary
)
if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_Fram PRIVATE Fram.cpp)
    target_link_libraries(Sts1CobcSw_Fram PRIVATE Sts1CobcSw_Hal)
else()
    target_sources(Sts1CobcSw_Fram PRIVATE FramMock.cpp)
endif()
