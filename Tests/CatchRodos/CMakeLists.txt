add_library(Sts1CobcSwTests_CatchRodos INTERFACE)
add_library(Sts1CobcSwTests::CatchRodos ALIAS Sts1CobcSwTests_CatchRodos)
# For some reason it doesn't work if TestThread.cpp is added as a private source to the library. It
# must be a source of the final executable, or Rodos won't find and execute the thread.
target_sources(Sts1CobcSwTests_CatchRodos INTERFACE TestThread.cpp)
target_link_libraries(
    Sts1CobcSwTests_CatchRodos INTERFACE etl::etl rodos::rodos Sts1CobcSwTests::HardwareSetup
)
target_include_directories(
    Sts1CobcSwTests_CatchRodos INTERFACE "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>"
)

add_program(CatchRodos_Example Example.test.cpp)
target_link_libraries(Sts1CobcSwTests_CatchRodos_Example PRIVATE Sts1CobcSwTests::CatchRodos)

add_program(CatchRodos_FailMessages FailMessages.test.cpp)
target_link_libraries(Sts1CobcSwTests_CatchRodos_FailMessages PRIVATE Sts1CobcSwTests::CatchRodos)
