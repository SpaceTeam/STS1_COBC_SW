# ---- Tests only for Linux ----

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
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
        Sts1CobcSwTests_SpiSupervisor
        PRIVATE rodos::rodos Sts1CobcSw_FramSections Sts1CobcSw_Hal Sts1CobcSw_Periphery
                Sts1CobcSw_Serial Sts1CobcSw_Utility
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
