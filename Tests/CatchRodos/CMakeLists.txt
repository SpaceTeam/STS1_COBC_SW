add_library(Sts1CobcSwTests_CatchRodos STATIC)
add_library(Sts1CobcSwTests::CatchRodos ALIAS Sts1CobcSwTests_CatchRodos)
target_sources(Sts1CobcSwTests_CatchRodos PRIVATE RfLatchupDisablePin.cpp)
# For some reason if doesn't work if TestThread.cpp is added as a private source to the library. It
# must be a source of the final executable, or Rodos won't find and execute the thread.
target_sources(Sts1CobcSwTests_CatchRodos INTERFACE TestThread.cpp)
target_link_libraries(Sts1CobcSwTests_CatchRodos INTERFACE rodos::rodos etl::etl)
if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    target_link_libraries(Sts1CobcSwTests_CatchRodos PRIVATE Sts1CobcSw_Hal)
endif()
target_include_directories(
    Sts1CobcSwTests_CatchRodos PUBLIC ${warning_guard} "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>"
)

add_executable(Sts1CobcSwTests_CatchRodos_Example Example.test.cpp)
target_link_libraries(Sts1CobcSwTests_CatchRodos_Example PRIVATE Sts1CobcSwTests::CatchRodos)

add_executable(Sts1CobcSwTests_CatchRodos_FailMessages FailMessages.test.cpp)
target_link_libraries(Sts1CobcSwTests_CatchRodos_FailMessages PRIVATE Sts1CobcSwTests::CatchRodos)

add_dependencies(AllTests Sts1CobcSwTests_CatchRodos)
add_dependencies(AllTests Sts1CobcSwTests_CatchRodos_Example)
add_dependencies(AllTests Sts1CobcSwTests_CatchRodos_FailMessages)
