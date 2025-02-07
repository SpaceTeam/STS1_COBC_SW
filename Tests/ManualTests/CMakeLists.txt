add_program(DeviceIds DeviceIds.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_DeviceIds PRIVATE rodos::rodos Sts1CobcSw_Hal Sts1CobcSw_Periphery
                                      Sts1CobcSwTests::HardwareSetup
)

add_program(DisableRfLatchupProtection DisableRfLatchupProtection.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_DisableRfLatchupProtection PRIVATE rodos::rodos Sts1CobcSwTests::HardwareSetup
)

add_program(EduCommands EduCommands.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_EduCommands
    PRIVATE rodos::rodos
            strong_type::strong_type
            Sts1CobcSw_Edu
            Sts1CobcSw_Hal
            Sts1CobcSw_ProgramId
            Sts1CobcSw_Serial
            Sts1CobcSw_Utility
            Sts1CobcSwTests::HardwareSetup
)

add_program(Eps Eps.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Eps PRIVATE rodos::rodos Sts1CobcSw_Periphery Sts1CobcSwTests::HardwareSetup
)

add_program(Gpio Gpio.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Gpio PRIVATE rodos::rodos Sts1CobcSw_Hal Sts1CobcSwTests::HardwareSetup
)

add_program(MaxPower MaxPower.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_MaxPower
    PRIVATE rodos::rodos
            strong_type::strong_type
            Sts1CobcSw_Hal
            Sts1CobcSw_Periphery
            Sts1CobcSw_Serial
            Sts1CobcSw_Utility
            Sts1CobcSwTests::HardwareSetup
)

add_program(SpiSupervisor SpiSupervisor.test.cpp)
target_sources(
    Sts1CobcSwTests_SpiSupervisor
    PRIVATE ${CMAKE_SOURCE_DIR}/Sts1CobcSw/CobcSoftware/FlashStartupTestThread.cpp
            ${CMAKE_SOURCE_DIR}/Sts1CobcSw/CobcSoftware/FramEpsStartupTestThread.cpp
            ${CMAKE_SOURCE_DIR}/Sts1CobcSw/CobcSoftware/RfStartupTestThread.cpp
            ${CMAKE_SOURCE_DIR}/Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.cpp
)
target_link_libraries(
    Sts1CobcSwTests_SpiSupervisor
    PRIVATE rodos::rodos
            strong_type::strong_type
            Sts1CobcSw_FramSections
            Sts1CobcSw_Hal
            Sts1CobcSw_Periphery
            Sts1CobcSw_Serial
            Sts1CobcSw_Utility
            Sts1CobcSwTests::HardwareSetup
)

add_program(TemperatureSensor TemperatureSensor.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_TemperatureSensor PRIVATE rodos::rodos Sts1CobcSw_Periphery
                                              Sts1CobcSwTests::HardwareSetup
)

add_program(Uart Uart.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Uart
    PRIVATE rodos::rodos strong_type::strong_type Sts1CobcSw_Hal Sts1CobcSw_Serial
            Sts1CobcSw_Utility Sts1CobcSwTests::HardwareSetup
)

add_program(Watchdog Watchdog.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Watchdog
    PRIVATE rodos::rodos strong_type::strong_type Sts1CobcSw_Hal Sts1CobcSw_Utility
            Sts1CobcSwTests::HardwareSetupWithoutWatchdog
)

get_property(manual_test_targets DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BUILDSYSTEM_TARGETS)
add_custom_target(AllManualTests) # Must be defined after getting all manual test targets
add_dependencies(AllManualTests ${manual_test_targets})
