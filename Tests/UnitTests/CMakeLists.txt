# TODO: Add a target that runs all COBC tests. For this it needs all COBC tests as sources.

# ---- Tests for Linux and the COBC ----

add_test_program(Fram)
target_link_libraries(
    Sts1CobcSwTests_Fram PRIVATE rodos::rodos strong_type::strong_type Sts1CobcSw_Periphery
                                 Sts1CobcSw_Serial Sts1CobcSw_Utility Sts1CobcSwTests::CatchRodos
)
add_test(NAME Fram COMMAND Sts1CobcSwTests_Fram)

add_test_program(LfsWrapper)
target_link_libraries(
    Sts1CobcSwTests_LfsWrapper
    PRIVATE etl::etl littlefs::littlefs Sts1CobcSw_FileSystem Sts1CobcSw_Serial Sts1CobcSw_Utility
            Sts1CobcSwTests::CatchRodos
)
add_test(NAME LfsWrapper COMMAND Sts1CobcSwTests_LfsWrapper)

add_test_program(Littlefs)
target_link_libraries(
    Sts1CobcSwTests_Littlefs PRIVATE etl::etl littlefs::littlefs Sts1CobcSw_FileSystem
                                     Sts1CobcSw_Serial Sts1CobcSwTests::CatchRodos
)
add_test(NAME Littlefs COMMAND Sts1CobcSwTests_Littlefs)

# ---- Tests only for the COBC ----

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    add_test_program(Crc32)
    target_link_libraries(
        Sts1CobcSwTests_Crc32 PRIVATE Sts1CobcSw_Utility Sts1CobcSw_Serial
                                      Sts1CobcSwTests::CatchRodos
    )

    add_test_program(Flash)
    target_link_libraries(
        Sts1CobcSwTests_Flash PRIVATE Sts1CobcSw_Periphery Sts1CobcSw_Serial Sts1CobcSw_Utility
                                      strong_type::strong_type Sts1CobcSwTests::CatchRodos
    )

    add_test_program(WholeFram)
    target_link_libraries(
        Sts1CobcSwTests_WholeFram PRIVATE Sts1CobcSw_Periphery Sts1CobcSw_Serial Sts1CobcSw_Utility
                                          strong_type::strong_type Sts1CobcSwTests::CatchRodos
    )

    add_test_program(Rf)
    target_link_libraries(
        Sts1CobcSwTests_Rf PRIVATE Sts1CobcSw_Periphery Sts1CobcSwTests::CatchRodos
    )
endif()

# ---- Tests only for Linux ----

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    include(Catch)

    add_test_program(EdacVariable)
    target_link_libraries(
        Sts1CobcSwTests_EdacVariable PRIVATE Sts1CobcSw_Utility Sts1CobcSwTests::CatchRodos
    )
    add_test(NAME EdacVariable COMMAND Sts1CobcSwTests_EdacVariable)

    add_test_program(FlatArray)
    target_link_libraries(
        Sts1CobcSwTests_FlatArray PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Utility
    )
    catch_discover_tests(Sts1CobcSwTests_FlatArray)

    add_test_program(FramRingArray)
    target_link_libraries(
        Sts1CobcSwTests_FramRingArray
        PRIVATE etl::etl
                strong_type::strong_type
                Sts1CobcSw_FramSections
                Sts1CobcSw_Periphery
                Sts1CobcSw_Serial
                Sts1CobcSw_Utility
                Sts1CobcSwTests::CatchRodos
    )
    add_test(NAME FramRingArray COMMAND Sts1CobcSwTests_FramRingArray)

    add_test_program(FramVector)
    target_link_libraries(
        Sts1CobcSwTests_FramVector
        PRIVATE etl::etl
                strong_type::strong_type
                Sts1CobcSw_FramSections
                Sts1CobcSw_Periphery
                Sts1CobcSw_Serial
                Sts1CobcSw_Utility
                Sts1CobcSwTests::CatchRodos
    )
    add_test(NAME FramVector COMMAND Sts1CobcSwTests_FramVector)

    add_test_program(MajorityVote)
    target_link_libraries(
        Sts1CobcSwTests_MajorityVote PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Utility
    )
    catch_discover_tests(Sts1CobcSwTests_MajorityVote)

    add_test_program(Outcome)
    target_link_libraries(Sts1CobcSwTests_Outcome PRIVATE Catch2::Catch2WithMain)
    catch_discover_tests(Sts1CobcSwTests_Outcome)

    add_test_program(PersistentVariableInfo)
    target_link_libraries(
        Sts1CobcSwTests_PersistentVariableInfo
        PRIVATE Catch2::Catch2WithMain Sts1CobcSw_FramSections Sts1CobcSw_Periphery
                Sts1CobcSw_Serial Sts1CobcSw_Utility
    )
    catch_discover_tests(Sts1CobcSwTests_PersistentVariableInfo)

    add_test_program(PersistentVariables)
    target_link_libraries(
        Sts1CobcSwTests_PersistentVariables
        PRIVATE strong_type::strong_type Sts1CobcSw_FramSections Sts1CobcSw_Periphery
                Sts1CobcSw_Serial Sts1CobcSw_Utility Sts1CobcSwTests::CatchRodos
    )
    add_test(NAME PersistentVariables COMMAND Sts1CobcSwTests_PersistentVariables)

    add_test_program(Section)
    target_link_libraries(
        Sts1CobcSwTests_Section PRIVATE Catch2::Catch2WithMain Sts1CobcSw_FramSections
                                        Sts1CobcSw_Periphery
    )
    catch_discover_tests(Sts1CobcSwTests_Section)

    add_test_program(Serial)
    target_link_libraries(Sts1CobcSwTests_Serial PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Serial)
    catch_discover_tests(Sts1CobcSwTests_Serial)

    add_test_program(Span)
    target_link_libraries(Sts1CobcSwTests_Span PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Utility)
    catch_discover_tests(Sts1CobcSwTests_Span)

    add_test_program(StringLiteral)
    target_link_libraries(
        Sts1CobcSwTests_StringLiteral PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Utility
    )
    catch_discover_tests(Sts1CobcSwTests_StringLiteral)

    add_test_program(SubsectionInfo)
    target_link_libraries(
        Sts1CobcSwTests_SubsectionInfo PRIVATE Catch2::Catch2WithMain Sts1CobcSw_FramSections
                                               Sts1CobcSw_Periphery Sts1CobcSw_Utility
    )
    catch_discover_tests(Sts1CobcSwTests_SubsectionInfo)

    add_test_program(Subsections)
    target_link_libraries(
        Sts1CobcSwTests_Subsections PRIVATE Catch2::Catch2WithMain Sts1CobcSw_FramSections
                                            Sts1CobcSw_Periphery Sts1CobcSw_Utility
    )
    catch_discover_tests(Sts1CobcSwTests_Subsections)

    add_test_program(TelemetryRecord)
    target_link_libraries(
        Sts1CobcSwTests_TelemetryRecord PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Serial
                                                Sts1CobcSw_Telemetry
    )
    catch_discover_tests(Sts1CobcSwTests_TelemetryRecord)
endif()

# ---- All unit tests ----

get_property(unit_test_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
add_custom_target(AllUnitTests) # Must be defined after getting all unit test targets
add_dependencies(AllUnitTests ${unit_test_targets})
