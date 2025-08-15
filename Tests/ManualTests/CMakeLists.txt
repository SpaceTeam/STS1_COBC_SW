# ---- Tests only for the COBC ----

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    add_test_program(DeviceIds)
    target_link_libraries(
        Sts1CobcSwTests_DeviceIds
        PRIVATE rodos::rodos
                Sts1CobcSw_Flash
                Sts1CobcSw_Fram
                Sts1CobcSw_Hal
                Sts1CobcSw_Outcome
                Sts1CobcSw_Rf
                Sts1CobcSwTests::HardwareSetup
    )

    add_test_program(DisableRfLatchupProtection DisableRfLatchupProtection)
    target_link_libraries(
        Sts1CobcSwTests_DisableRfLatchupProtection PRIVATE rodos::rodos
                                                           Sts1CobcSwTests::HardwareSetup
    )

    add_test_program(EduCommands)
    target_link_libraries(
        Sts1CobcSwTests_EduCommands
        PRIVATE rodos::rodos
                strong_type::strong_type
                Sts1CobcSw_Edu
                Sts1CobcSw_Hal
                Sts1CobcSw_Outcome
                Sts1CobcSw_RealTime
                Sts1CobcSw_Serial
                Sts1CobcSw_Vocabulary
                Sts1CobcSwTests::HardwareSetup
    )

    add_test_program(Eps)
    target_link_libraries(
        Sts1CobcSwTests_Eps
        PRIVATE rodos::rodos Sts1CobcSw_Fram Sts1CobcSw_FramSections Sts1CobcSw_Sensors
                Sts1CobcSw_Serial Sts1CobcSwTests::HardwareSetup
    )

    add_test_program(Gpio)
    target_link_libraries(
        Sts1CobcSwTests_Gpio
        PRIVATE rodos::rodos strong_type::strong_type Sts1CobcSw_Hal Sts1CobcSw_RodosTime
                Sts1CobcSw_Vocabulary Sts1CobcSwTests::HardwareSetup
    )

    add_test_program(MaxPower)
    target_link_libraries(
        Sts1CobcSwTests_MaxPower
        PRIVATE rodos::rodos
                strong_type::strong_type
                Sts1CobcSw_Fram
                Sts1CobcSw_Hal
                Sts1CobcSw_Serial
                Sts1CobcSw_Utility
                Sts1CobcSw_Vocabulary
                Sts1CobcSwTests::HardwareSetup
    )

    add_test_program(RfReceive)
    target_link_libraries(
        Sts1CobcSwTests_RfReceive
        PRIVATE rodos::rodos
                strong_type::strong_type
                Sts1CobcSw_Fram
                Sts1CobcSw_Hal
                Sts1CobcSw_Outcome
                Sts1CobcSw_Rf
                Sts1CobcSw_Serial
                Sts1CobcSw_Utility
                Sts1CobcSw_Vocabulary
                Sts1CobcSwTests::HardwareSetup
    )

    add_test_program(RfSend)
    target_link_libraries(
        Sts1CobcSwTests_RfSend
        PRIVATE rodos::rodos
                strong_type::strong_type
                Sts1CobcSw_Fram
                Sts1CobcSw_Hal
                Sts1CobcSw_Outcome
                Sts1CobcSw_Rf
                Sts1CobcSw_RodosTime
                Sts1CobcSw_Serial
                Sts1CobcSw_Utility
                Sts1CobcSw_Vocabulary
                Sts1CobcSwTests::HardwareSetup
    )

    add_test_program(SpiSupervisor)
    target_sources(
        Sts1CobcSwTests_SpiSupervisor
        PRIVATE ${CMAKE_SOURCE_DIR}/Sts1CobcSw/Firmware/FlashStartupTestThread.cpp
                ${CMAKE_SOURCE_DIR}/Sts1CobcSw/Firmware/FramEpsStartupTestThread.cpp
                ${CMAKE_SOURCE_DIR}/Sts1CobcSw/Firmware/RfStartupTestThread.cpp
                ${CMAKE_SOURCE_DIR}/Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.cpp
    )
    target_link_libraries(
        Sts1CobcSwTests_SpiSupervisor
        PRIVATE rodos::rodos
                strong_type::strong_type
                Sts1CobcSw_Edu
                Sts1CobcSw_FileSystem
                Sts1CobcSw_Flash
                Sts1CobcSw_Fram
                Sts1CobcSw_FramSections
                Sts1CobcSw_Hal
                Sts1CobcSw_Serial
                Sts1CobcSw_Rf
                Sts1CobcSw_RodosTime
                Sts1CobcSw_Sensors
                Sts1CobcSw_Utility
                Sts1CobcSw_Vocabulary
                Sts1CobcSw_WatchdogTimers
                Sts1CobcSwTests::HardwareSetup
    )

    add_test_program(TemperatureSensor)
    target_link_libraries(
        Sts1CobcSwTests_TemperatureSensor PRIVATE rodos::rodos Sts1CobcSw_Sensors
                                                  Sts1CobcSwTests::HardwareSetup
    )

    add_test_program(Uart)
    target_link_libraries(
        Sts1CobcSwTests_Uart
        PRIVATE rodos::rodos
                strong_type::strong_type
                Sts1CobcSw_Hal
                Sts1CobcSw_Serial
                Sts1CobcSw_RodosTime
                Sts1CobcSw_Utility
                Sts1CobcSwTests::HardwareSetup
    )

    add_test_program(Watchdog)
    target_link_libraries(
        Sts1CobcSwTests_Watchdog
        PRIVATE rodos::rodos strong_type::strong_type Sts1CobcSw_Hal Sts1CobcSw_RodosTime
                Sts1CobcSw_Vocabulary Sts1CobcSwTests::HardwareSetupWithoutWatchdog
    )
endif()

# ---- Tests only for Linux ----

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    add_test_program(ReportFrames)
    target_link_libraries(
        Sts1CobcSwTests_ReportFrames
        PRIVATE etl::etl
                rodos::rodos
                strong_type::strong_type
                Sts1CobcSw_FileSystem
                Sts1CobcSw_FirmwareManagement
                Sts1CobcSw_Fram
                Sts1CobcSw_Outcome
                Sts1CobcSw_RfProtocols
                Sts1CobcSw_RodosTime
                Sts1CobcSw_Serial
                Sts1CobcSw_Telemetry
                Sts1CobcSw_Utility
                Sts1CobcSw_Vocabulary
    )
endif()

get_property(manual_test_targets DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BUILDSYSTEM_TARGETS)
add_custom_target(AllManualTests)
if(manual_test_targets)
    add_dependencies(AllManualTests ${manual_test_targets})
endif()
