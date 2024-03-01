add_subdirectory(ThreadTests)

add_library(Sts1CobcSwTests_Utility STATIC Utility.cpp)
target_link_libraries(Sts1CobcSwTests_Utility PRIVATE rodos::rodos)
target_include_directories(
    Sts1CobcSwTests_Utility PUBLIC ${warning_guard} "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>"
)

add_program(Crc32 Crc32.test.cpp)
target_link_libraries(Sts1CobcSwTests_Crc32 PRIVATE rodos::rodos Sts1CobcSw_Utility Sts1CobcSwTests_Utility)

add_program(EduCommands EduCommands.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_EduCommands PRIVATE rodos::rodos Sts1CobcSw_Utility Sts1CobcSw_Serial
                                        Sts1CobcSw_Hal Sts1CobcSw_Edu
)

add_program(Eps Eps.test.cpp)
target_link_libraries(Sts1CobcSwTests_Eps PRIVATE rodos::rodos Sts1CobcSw_Periphery)

add_program(FileSystem FileSystem.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_FileSystem PRIVATE rodos::rodos littlefs::littlefs Sts1CobcSw_FileSystem
)

add_program(Flash Flash.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Flash PRIVATE rodos::rodos Sts1CobcSw_Periphery Sts1CobcSw_Serial
                                  Sts1CobcSwTests_Utility
)

add_program(Fram Fram.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Fram PRIVATE rodos::rodos Sts1CobcSw_Periphery Sts1CobcSw_Serial
                                 Sts1CobcSw_Utility Sts1CobcSwTests_Utility
)

add_program(Gpio Gpio.test.cpp)
target_link_libraries(Sts1CobcSwTests_Gpio PRIVATE rodos::rodos Sts1CobcSw_Hal)

add_program(Rf Rf.test.cpp)
target_link_libraries(Sts1CobcSwTests_Rf PRIVATE rodos::rodos Sts1CobcSw_Periphery Sts1CobcSwTests_Utility)

add_program(Uart Uart.test.cpp)
target_link_libraries(Sts1CobcSwTests_Uart PRIVATE rodos::rodos Sts1CobcSw_Hal Sts1CobcSw_Utility)

add_program(Watchdog Watchdog.test.cpp)
target_link_libraries(Sts1CobcSwTests_Watchdog PRIVATE rodos::rodos Sts1CobcSw_Hal)

add_program(WatchdogClear Watchdog.test.cpp WatchdogClear.test.cpp)
target_link_libraries(Sts1CobcSwTests_WatchdogClear PRIVATE rodos::rodos Sts1CobcSw_Hal)

get_property(
    top_level_hw_test_targets
    DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    PROPERTY BUILDSYSTEM_TARGETS
)

add_custom_target(AllHardwareTests) # Must be defined after getting all hardware test targets
add_dependencies(AllHardwareTests ${top_level_hw_test_targets} AllThreadTests)
