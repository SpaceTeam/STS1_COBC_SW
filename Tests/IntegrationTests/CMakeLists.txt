# ---- Tests only for Linux ----

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    include("${CMAKE_SOURCE_DIR}/cmake/add-golden-test.cmake")

    add_golden_test(HelloWorld)
    target_link_libraries(Sts1CobcSwTests_HelloWorld PRIVATE rodos::rodos)

    add_test_program(SingleBufferMultiThreaded)
    target_link_libraries(
        Sts1CobcSwTests_SingleBufferMultiThreaded
        PRIVATE rodos::rodos strong_type::strong_type Sts1CobcSw_SingleBuffer Sts1CobcSw_RodosTime
                Sts1CobcSw_Vocabulary
    )
    add_test(NAME SingleBufferMultiThreaded COMMAND Sts1CobcSwTests_SingleBufferMultiThreaded)

    add_golden_test(SpiSupervisor)
    target_sources(
        Sts1CobcSwTests_SpiSupervisor
        PRIVATE ${CMAKE_SOURCE_DIR}/Sts1CobcSw/Firmware/StartupTestThreadStubs.cpp
                ${CMAKE_SOURCE_DIR}/Sts1CobcSw/Firmware/SpiStartupTestAndSupervisorThread.cpp
    )
    target_link_libraries(
        Sts1CobcSwTests_SpiSupervisor
        PRIVATE rodos::rodos
                Sts1CobcSw_Fram
                Sts1CobcSw_FramSections
                Sts1CobcSw_Hal
                Sts1CobcSw_RodosTime
                Sts1CobcSw_Serial
                Sts1CobcSw_Utility
    )
endif()

# ---- All integration tests ----

get_property(
    integration_test_targets DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BUILDSYSTEM_TARGETS
)
add_custom_target(AllIntegrationTests)
if(integration_test_targets)
    add_dependencies(AllIntegrationTests ${integration_test_targets})
endif()
