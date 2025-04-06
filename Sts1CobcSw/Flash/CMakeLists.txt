target_link_libraries(
    Sts1CobcSw_Flash PUBLIC Sts1CobcSw_Outcome Sts1CobcSw_Serial Sts1CobcSw_Vocabulary
)
if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_Flash PRIVATE Flash.cpp)
    target_link_libraries(
        Sts1CobcSw_Flash PRIVATE strong_type::strong_type Sts1CobcSw_Hal Sts1CobcSw_RodosTime
                                 Sts1CobcSw_Utility
    )
else()
    target_sources(Sts1CobcSw_Flash PRIVATE FlashMock.cpp)
endif()
