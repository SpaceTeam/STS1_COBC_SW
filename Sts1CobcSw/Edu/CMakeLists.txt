target_sources(Sts1CobcSw_Edu PRIVATE ProgramQueue.cpp ProgramStatusHistory.cpp)
target_link_libraries(
    Sts1CobcSw_Edu PUBLIC rodos::rodos etl::etl Sts1CobcSw_FramSections Sts1CobcSw_Serial
                          Sts1CobcSw_Outcome
)
target_link_libraries(Sts1CobcSw_Edu PRIVATE Sts1CobcSw_Utility)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_Edu PRIVATE Edu.cpp)
    target_link_libraries(
        Sts1CobcSw_Edu PUBLIC Sts1CobcSw_Hal Sts1CobcSw_Periphery Sts1CobcSw_FileSystem
    )
endif()
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_sources(Sts1CobcSw_Edu PRIVATE EduMock.cpp)
endif()
