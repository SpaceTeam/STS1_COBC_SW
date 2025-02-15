add_subdirectory(ThreadTests)

add_program(Crc32 Crc32.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Crc32 PRIVATE Sts1CobcSw_Utility Sts1CobcSw_Serial Sts1CobcSwTests::CatchRodos
)

add_program(Flash Flash.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Flash PRIVATE Sts1CobcSw_Periphery Sts1CobcSw_Serial Sts1CobcSw_Utility
                                  strong_type::strong_type Sts1CobcSwTests::CatchRodos
)

add_program(Fram Fram.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Fram
    PRIVATE rodos::rodos Sts1CobcSw_Periphery Sts1CobcSw_Serial Sts1CobcSw_Utility
            strong_type::strong_type Sts1CobcSwTests::CatchRodos
)

add_program(WholeFram WholeFram.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_WholeFram PRIVATE Sts1CobcSw_Periphery Sts1CobcSw_Serial Sts1CobcSw_Utility
                                      strong_type::strong_type Sts1CobcSwTests::CatchRodos
)

add_program(Littlefs Littlefs.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Littlefs PRIVATE etl::etl littlefs::littlefs Sts1CobcSw_FileSystem
                                     Sts1CobcSwTests::CatchRodos
)

add_program(Rf Rf.test.cpp)
target_link_libraries(Sts1CobcSwTests_Rf PRIVATE Sts1CobcSw_Periphery Sts1CobcSwTests::CatchRodos)

get_property(
    top_level_hw_test_targets DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BUILDSYSTEM_TARGETS
)
add_custom_target(AllHardwareTests) # Must be defined after getting all hardware test targets
add_dependencies(AllHardwareTests ${top_level_hw_test_targets} AllThreadTests)
