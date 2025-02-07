add_subdirectory(ThreadTests)

add_library(Sts1CobcSwTests_Utility STATIC Utility.cpp)
target_link_libraries(Sts1CobcSwTests_Utility PRIVATE rodos::rodos)
target_include_directories(
    Sts1CobcSwTests_Utility PUBLIC ${warning_guard} "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>"
)

add_program(Crc32 Crc32.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Crc32 PRIVATE rodos::rodos Sts1CobcSw_Utility Sts1CobcSwTests_Utility
                                  Sts1CobcSwTests_RfLatchupDisablePin
)

add_program(Flash Flash.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Flash
    PRIVATE rodos::rodos
            Sts1CobcSw_Periphery
            Sts1CobcSw_Serial
            Sts1CobcSw_Utility
            strong_type::strong_type
            Sts1CobcSwTests::HardwareSetup
            Sts1CobcSwTests_Utility
)

add_program(Fram Fram.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Fram
    PRIVATE rodos::rodos
            Sts1CobcSw_Periphery
            Sts1CobcSw_Serial
            Sts1CobcSw_Utility
            strong_type::strong_type
            Sts1CobcSwTests::HardwareSetup
            Sts1CobcSwTests_Utility
)

add_program(Littlefs Littlefs.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Littlefs PRIVATE littlefs::littlefs rodos::rodos Sts1CobcSw_FileSystem
                                     Sts1CobcSwTests::HardwareSetup Sts1CobcSwTests_Utility
)

add_program(Rf Rf.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Rf PRIVATE rodos::rodos Sts1CobcSw_Periphery Sts1CobcSwTests::HardwareSetup
                               Sts1CobcSwTests_Utility
)

get_property(
    top_level_hw_test_targets DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BUILDSYSTEM_TARGETS
)
add_custom_target(AllHardwareTests) # Must be defined after getting all hardware test targets
add_dependencies(AllHardwareTests ${top_level_hw_test_targets} AllThreadTests)
