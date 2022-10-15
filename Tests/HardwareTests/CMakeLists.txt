add_program(Gpio Gpio.test.cpp)
target_link_libraries(Sts1CobcSwTests_Gpio PUBLIC rodos::rodos type_safe Sts1CobcSw_Hal)

add_program(Uart Uart.test.cpp)
target_link_libraries(Sts1CobcSwTests_Uart PUBLIC rodos::rodos Sts1CobcSw_Hal)

add_program(Spi Spi.test.cpp)
target_link_libraries(Sts1CobcSwTests_Spi PUBLIC rodos::rodos type_safe Sts1CobcSw_Hal)

get_property(
    all_hw_test_targets
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY BUILDSYSTEM_TARGETS
)
add_custom_target(AllHardwareTests) # Must be defined after getting all hardware test targets
add_dependencies(AllHardwareTests ${all_hw_test_targets})
