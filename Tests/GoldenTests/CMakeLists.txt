include(${CMAKE_SOURCE_DIR}/cmake/golden-tests.cmake)

# Initialize "global" variables used by macros
set(output_files "")

set(Sts1CobcSw "${CMAKE_SOURCE_DIR}/Sts1CobcSw")

add_golden_test(
    TESTFILE "HelloWorld.test.cpp" SOURCE "${Sts1CobcSw}/TopicsAndSubscribers.cpp" LIB rodos::rodos
)
add_golden_test(TESTFILE "HelloDummy.test.cpp" LIB rodos::rodos Sts1CobcSw_Dummy)

add_golden_test(
    TESTFILE
    "DispatchCommand.test.cpp"
    SOURCE
    "${Sts1CobcSw}/CommandParser.cpp"
    "${Sts1CobcSw}/EduProgramQueue.cpp"
    "${Sts1CobcSw}/EduProgramStatusHistory.cpp"
    LIB
    rodos::rodos
    etl::etl
    Sts1CobcSw_Periphery
    Sts1CobcSw_Utility
    Sts1CobcSw_Serial
)

add_custom_target(AllGoldenTests DEPENDS ${output_files})
