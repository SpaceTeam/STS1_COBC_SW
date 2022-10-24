add_subdirectory(EduComandTests)

add_program(Gpio Gpio.test.cpp)
target_link_libraries(Sts1CobcSwTests_Gpio PRIVATE rodos::rodos type_safe Sts1CobcSw_Hal)

add_program(Spi Spi.test.cpp)
target_link_libraries(Sts1CobcSwTests_Spi PRIVATE etl::etl rodos::rodos type_safe Sts1CobcSw_Hal)

add_program(Uart Uart.test.cpp)
target_link_libraries(Sts1CobcSwTests_Uart PRIVATE rodos::rodos Sts1CobcSw_Hal)

add_program(Crc32 Crc32.test.cpp)
target_link_libraries(Sts1CobcSwTests_Crc32 PRIVATE rodos::rodos)

get_property(
    all_hw_test_targets
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY BUILDSYSTEM_TARGETS
)
add_custom_target(AllHardwareTests) # Must be defined after getting all hardware test targets
add_dependencies(AllHardwareTests ${all_hw_test_targets})
