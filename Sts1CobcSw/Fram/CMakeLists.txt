target_link_libraries(
    Sts1CobcSw_Fram
    PUBLIC rodos::rodos strong_type::strong_type Sts1CobcSw_ErrorDetectionAndCorrection
           Sts1CobcSw_Serial Sts1CobcSw_Vocabulary
)
if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_Fram PRIVATE Fram.cpp)
    target_link_libraries(Sts1CobcSw_Fram PRIVATE Sts1CobcSw_Hal Sts1CobcSw_Utility)
else()
    target_sources(Sts1CobcSw_Fram PRIVATE FramMock.cpp)
endif()
