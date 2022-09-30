add_subdirectory(Hal)
add_subdirectory(Util)
add_subdirectory(Periphery)

target_sources(Sts1CobcSw_Dummy PRIVATE Dummy.cpp)
target_link_libraries(Sts1CobcSw_Dummy PUBLIC etl::etl)

target_sources(Sts1CobcSw_HelloDummy PRIVATE HelloDummy.cpp)
target_link_libraries(Sts1CobcSw_HelloDummy PRIVATE Sts1CobcSw_Dummy rodos::rodos)

target_sources(Sts1CobcSw_EduProgramQueueThread PRIVATE EduProgramQueueThread.cpp CommandParser.cpp CommandParser.hpp CobcCommands.cpp CobcCommands.hpp)
target_link_libraries(Sts1CobcSw_EduProgramQueueThread PRIVATE rodos::rodos Sts1CobcSw_Hal Sts1CobcSw_Util type_safe Sts1CobcSw_Periphery)

