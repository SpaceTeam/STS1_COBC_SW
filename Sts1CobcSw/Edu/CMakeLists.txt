target_sources(Sts1CobcSw_Edu PRIVATE ProgramStatusHistory.cpp)
target_link_libraries(
    Sts1CobcSw_Edu PUBLIC rodos::without_main_on_linux etl::etl Sts1CobcSw_FramSections
                          Sts1CobcSw_Outcome Sts1CobcSw_Serial Sts1CobcSw_Vocabulary
)
target_link_libraries(Sts1CobcSw_Edu PRIVATE Sts1CobcSw_Utility)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(Sts1CobcSw_Edu PRIVATE Edu.cpp)
    target_link_libraries(
        Sts1CobcSw_Edu PRIVATE littlefs::littlefs Sts1CobcSw_FileSystem Sts1CobcSw_Hal
                               Sts1CobcSw_Periphery Sts1CobcSw_RodosTime
    )
endif()
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_sources(Sts1CobcSw_Edu PRIVATE EduMock.cpp)
    target_link_libraries(Sts1CobcSw_Edu PRIVATE Sts1CobcSw_RealTime)
endif()
