add_subdirectory(ThreadTests)

function(add_watchdog_version_of program_name)
    set(target_name ${PROJECT_NAME}_${program_name})
    get_property(sources TARGET ${target_name} PROPERTY SOURCES)
    get_property(libraries TARGET ${target_name} PROPERTY LINK_LIBRARIES)
    add_program(${program_name}WithWatchdog ${sources} WatchdogClear.test.cpp)
    list(APPEND libraries rodos::rodos Sts1CobcSw_Hal)
    list(REMOVE_DUPLICATES libraries)
    target_link_libraries(${target_name}WithWatchdog PRIVATE ${libraries})
endfunction()

add_library(Sts1CobcSwTests_Utility STATIC Utility.cpp)
target_link_libraries(Sts1CobcSwTests_Utility PRIVATE rodos::rodos)
target_include_directories(
    Sts1CobcSwTests_Utility PUBLIC ${warning_guard} "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>"
)

add_library(Sts1CobcSwTests_RfLatchupDisablePin STATIC RfLatchupDisablePin.cpp)
target_link_libraries(Sts1CobcSwTests_RfLatchupDisablePin PUBLIC Sts1CobcSw_Hal)
target_include_directories(
    Sts1CobcSwTests_RfLatchupDisablePin PUBLIC ${warning_guard}
                                               "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>"
)

add_program(Crc32 Crc32.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Crc32 PRIVATE rodos::rodos Sts1CobcSw_Utility Sts1CobcSwTests_Utility
                                  Sts1CobcSwTests_RfLatchupDisablePin
)
add_watchdog_version_of(Crc32)

add_program(DeviceIds DeviceIds.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_DeviceIds PRIVATE rodos::rodos Sts1CobcSw_Hal Sts1CobcSw_Periphery
                                      Sts1CobcSw_Serial Sts1CobcSwTests_RfLatchupDisablePin
)
add_watchdog_version_of(DeviceIds)

add_program(DisableRfLatchupProtection DisableRfLatchupProtection.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_DisableRfLatchupProtection PRIVATE rodos::rodos
                                                       Sts1CobcSwTests_RfLatchupDisablePin
)
add_watchdog_version_of(DisableRfLatchupProtection)

add_program(EduCommands EduCommands.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_EduCommands
    PRIVATE rodos::rodos Sts1CobcSw_Utility Sts1CobcSw_Serial Sts1CobcSw_Hal Sts1CobcSw_Edu
            Sts1CobcSwTests_RfLatchupDisablePin
)
add_watchdog_version_of(EduCommands)

add_program(Eps Eps.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Eps PRIVATE rodos::rodos Sts1CobcSw_Periphery
                                Sts1CobcSwTests_RfLatchupDisablePin
)
add_watchdog_version_of(Eps)

add_program(Flash Flash.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Flash PRIVATE rodos::rodos Sts1CobcSw_Periphery Sts1CobcSw_Serial
                                  Sts1CobcSwTests_Utility Sts1CobcSwTests_RfLatchupDisablePin
)
add_watchdog_version_of(Flash)

add_program(Fram Fram.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Fram
    PRIVATE rodos::rodos Sts1CobcSw_Periphery Sts1CobcSw_Serial Sts1CobcSw_Utility
            Sts1CobcSwTests_Utility Sts1CobcSwTests_RfLatchupDisablePin
)
add_watchdog_version_of(Fram)

add_program(MaxPower MaxPower.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_MaxPower
    PRIVATE rodos::rodos Sts1CobcSw_Hal Sts1CobcSw_Periphery Sts1CobcSw_Serial Sts1CobcSw_Utility
            Sts1CobcSwTests_RfLatchupDisablePin
)
add_watchdog_version_of(MaxPower)

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
            Sts1CobcSwTests_RfLatchupDisablePin
)
add_watchdog_version_of(SpiSupervisor)

add_program(TemperatureSensor TemperatureSensor.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_TemperatureSensor PRIVATE rodos::rodos Sts1CobcSw_Periphery
                                              Sts1CobcSwTests_RfLatchupDisablePin
)
add_watchdog_version_of(TemperatureSensor)

add_program(Gpio Gpio.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Gpio PRIVATE rodos::rodos Sts1CobcSw_Hal Sts1CobcSwTests_RfLatchupDisablePin
)
add_watchdog_version_of(Gpio)

add_program(Littlefs Littlefs.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Littlefs PRIVATE littlefs::littlefs rodos::rodos Sts1CobcSw_FileSystem
                                     Sts1CobcSwTests_RfLatchupDisablePin Sts1CobcSwTests_Utility
)
add_watchdog_version_of(Littlefs)

add_program(Rf Rf.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Rf PRIVATE rodos::rodos Sts1CobcSw_Periphery Sts1CobcSwTests_Utility
                               Sts1CobcSwTests_RfLatchupDisablePin
)
add_watchdog_version_of(Rf)

add_program(Uart Uart.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Uart PRIVATE rodos::rodos Sts1CobcSw_Hal Sts1CobcSw_Utility
                                 Sts1CobcSwTests_RfLatchupDisablePin
)
add_watchdog_version_of(Uart)

add_program(Watchdog Watchdog.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Watchdog PRIVATE rodos::rodos Sts1CobcSw_Hal Sts1CobcSw_Utility
                                     Sts1CobcSwTests_RfLatchupDisablePin
)

add_program(WatchdogClear Watchdog.test.cpp WatchdogClear.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_WatchdogClear PRIVATE rodos::rodos Sts1CobcSw_Hal
                                          Sts1CobcSwTests_RfLatchupDisablePin
)

get_property(
    top_level_hw_test_targets DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BUILDSYSTEM_TARGETS
)

add_custom_target(AllHardwareTests) # Must be defined after getting all hardware test targets
add_dependencies(AllHardwareTests ${top_level_hw_test_targets} AllThreadTests)
