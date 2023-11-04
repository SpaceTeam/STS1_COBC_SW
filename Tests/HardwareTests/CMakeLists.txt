add_subdirectory(EduCommandTests)
add_subdirectory(ThreadTests)

add_library(Sts1CobcSwTests_Utility STATIC Utility.cpp)
target_link_libraries(Sts1CobcSwTests_Utility PRIVATE rodos::rodos)
target_include_directories(
    Sts1CobcSwTests_Utility PUBLIC ${warning_guard} "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>"
)

add_program(Crc32 Crc32.test.cpp)
target_link_libraries(Sts1CobcSwTests_Crc32 PRIVATE rodos::rodos)

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
                                 Sts1CobcSwTests_Utility
)

add_program(Gpio Gpio.test.cpp)
target_link_libraries(Sts1CobcSwTests_Gpio PRIVATE rodos::rodos type_safe Sts1CobcSw_Hal)

add_program(Spi Spi.test.cpp)
target_link_libraries(Sts1CobcSwTests_Spi PRIVATE etl::etl rodos::rodos type_safe Sts1CobcSw_Hal)

add_program(Uart Uart.test.cpp)
target_link_libraries(Sts1CobcSwTests_Uart PRIVATE rodos::rodos Sts1CobcSw_Hal)

get_property(
    top_level_hw_test_targets
    DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    PROPERTY BUILDSYSTEM_TARGETS
)

add_custom_target(AllHardwareTests) # Must be defined after getting all hardware test targets
add_dependencies(AllHardwareTests ${top_level_hw_test_targets} AllEduCommandTests AllThreadTests)
