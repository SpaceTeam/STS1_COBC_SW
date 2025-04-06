if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_Rf PRIVATE Rf.cpp)
    target_link_libraries(
        Sts1CobcSw_Rf PRIVATE strong_type::strong_type Sts1CobcSw_Hal Sts1CobcSw_RodosTime
                              Sts1CobcSw_Serial Sts1CobcSw_Utility Sts1CobcSw_Vocabulary
    )
else()
    target_sources(Sts1CobcSw_Rf PRIVATE RfMock.cpp)
endif()
