include("${CMAKE_SOURCE_DIR}/cmake/add-golden-test.cmake")

add_golden_test(HelloWorld.test.cpp)
target_link_libraries(Sts1CobcSwTests_HelloWorld PRIVATE rodos::rodos)

add_golden_test(SpiSupervisor.test.cpp)
target_sources(
    Sts1CobcSwTests_SpiSupervisor
    PRIVATE ${CMAKE_SOURCE_DIR}/Sts1CobcSw/CobcSoftware/StartupTestThreadStubs.cpp
            ${CMAKE_SOURCE_DIR}/Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.cpp
)
target_link_libraries(
    Sts1CobcSwTests_SpiSupervisor PRIVATE rodos::rodos Sts1CobcSw_FramSections Sts1CobcSw_Hal
                                          Sts1CobcSw_Periphery Sts1CobcSw_Serial Sts1CobcSw_Utility
)

# ---- All golden tests ----

get_property(golden_test_targets DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BUILDSYSTEM_TARGETS)
add_custom_target(AllGoldenTests) # Must be defined after getting all hardware test targets
add_dependencies(AllGoldenTests ${golden_test_targets})
