if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    add_subdirectory(Hal)
    add_subdirectory(Periphery)
endif()
add_subdirectory(Serial)
add_subdirectory(Utility)

target_sources(Sts1CobcSw_Dummy PRIVATE Dummy.cpp)
target_link_libraries(Sts1CobcSw_Dummy PUBLIC etl::etl)

target_sources(Sts1CobcSw_HelloDummy PRIVATE HelloDummy.cpp)
target_link_libraries(
    Sts1CobcSw_HelloDummy PRIVATE Sts1CobcSw_Dummy rodos::rodos Sts1CobcSw_Utility
)

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_sources(
        Sts1CobcSw_Sts1CobcSw PRIVATE EduProgramQueue.cpp EduProgramQueueThread.cpp
                                      CobcCommands.cpp CommandParserThread.cpp
    )
    target_link_libraries(
        Sts1CobcSw_Sts1CobcSw PRIVATE etl::etl rodos::rodos type_safe Sts1CobcSw_Hal
                                      Sts1CobcSw_Utility Sts1CobcSw_Periphery
    )
endif()
