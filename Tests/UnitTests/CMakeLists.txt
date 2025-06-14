# ---- Tests for Linux and the COBC ----

add_test_program(Crc32)
target_link_libraries(
    Sts1CobcSwTests_Crc32 PRIVATE Sts1CobcSw_Utility Sts1CobcSw_Serial Sts1CobcSwTests::CatchRodos
)
add_test(NAME Crc32 COMMAND Sts1CobcSwTests_Crc32)

add_test_program(Fram)
target_link_libraries(
    Sts1CobcSwTests_Fram
    PRIVATE rodos::rodos
            strong_type::strong_type
            Sts1CobcSw_Fram
            Sts1CobcSw_Serial
            Sts1CobcSw_Utility
            Sts1CobcSw_Vocabulary
            Sts1CobcSwTests::CatchRodos
)
add_test(NAME Fram COMMAND Sts1CobcSwTests_Fram)

add_test_program(FileSystem)
target_link_libraries(
    Sts1CobcSwTests_FileSystem
    PRIVATE etl::etl
            littlefs::littlefs
            Sts1CobcSw_FileSystem
            Sts1CobcSw_Fram
            Sts1CobcSw_FramSections
            Sts1CobcSw_Outcome
            Sts1CobcSw_Serial
            Sts1CobcSw_Utility
            Sts1CobcSwTests::CatchRodos
            Sts1CobcSwTests::Utility
)
add_test(NAME FileSystem COMMAND Sts1CobcSwTests_FileSystem)

add_test_program(Littlefs)
target_link_libraries(
    Sts1CobcSwTests_Littlefs PRIVATE etl::etl littlefs::littlefs Sts1CobcSw_FileSystem
                                     Sts1CobcSw_Serial Sts1CobcSwTests::CatchRodos
)
add_test(NAME Littlefs COMMAND Sts1CobcSwTests_Littlefs)

add_test_program(FirmwareManagement)
target_link_libraries(
    Sts1CobcSwTests_FirmwareManagement
    PRIVATE etl::etl
            rodos::rodos
            Sts1CobcSw_FirmwareManagement
            Sts1CobcSw_Outcome
            Sts1CobcSw_Serial
            Sts1CobcSw_Utility
            Sts1CobcSwTests::CatchRodos
            Sts1CobcSwTests::Utility
)
add_test(NAME FirmwareManagement COMMAND Sts1CobcSwTests_FirmwareManagement)

# ---- Tests only for the COBC ----

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    add_test_program(Flash)
    target_link_libraries(
        Sts1CobcSwTests_Flash PRIVATE strong_type::strong_type Sts1CobcSw_Flash Sts1CobcSw_Serial
                                      Sts1CobcSw_Vocabulary Sts1CobcSwTests::CatchRodos
    )

    add_test_program(WholeFram)
    target_link_libraries(
        Sts1CobcSwTests_WholeFram
        PRIVATE rodos::rodos
                strong_type::strong_type
                Sts1CobcSw_Fram
                Sts1CobcSw_Serial
                Sts1CobcSw_Utility
                Sts1CobcSw_Vocabulary
                Sts1CobcSwTests::CatchRodos
    )

    add_test_program(RfPartNumber)
    target_link_libraries(
        Sts1CobcSwTests_RfPartNumber PRIVATE Sts1CobcSw_Rf Sts1CobcSwTests::CatchRodos
    )
endif()

# ---- Tests only for Linux ----

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    include(Catch)

    add_test_program(Blake2s)
    target_link_libraries(
        Sts1CobcSwTests_Blake2s PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Blake2s Sts1CobcSw_Serial
    )
    catch_discover_tests(Sts1CobcSwTests_Blake2s)

    add_test_program(ChannelCoding)
    target_link_libraries(
        Sts1CobcSwTests_ChannelCoding PRIVATE Sts1CobcSw_ChannelCoding Sts1CobcSw_Outcome
                                              Sts1CobcSw_Serial Sts1CobcSwTests::CatchRodos
    )
    add_test(NAME ChannelCoding COMMAND Sts1CobcSwTests_ChannelCoding)

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
        PRIVATE strong_type::strong_type Sts1CobcSw_Fram Sts1CobcSw_FramSections Sts1CobcSw_Serial
                Sts1CobcSw_Utility Sts1CobcSwTests::CatchRodos
    )
    add_test(NAME FramRingArray COMMAND Sts1CobcSwTests_FramRingArray)

    add_test_program(FramVector)
    target_link_libraries(
        Sts1CobcSwTests_FramVector
        PRIVATE etl::etl
                strong_type::strong_type
                Sts1CobcSw_Fram
                Sts1CobcSw_FramSections
                Sts1CobcSw_Serial
                Sts1CobcSw_Utility
                Sts1CobcSwTests::CatchRodos
    )
    add_test(NAME FramVector COMMAND Sts1CobcSwTests_FramVector)

    add_test_program(Id)
    target_link_libraries(
        Sts1CobcSwTests_Id PRIVATE Sts1CobcSw_Outcome Sts1CobcSw_RfProtocols
                                   Sts1CobcSwTests::CatchRodos
    )
    add_test(NAME Id COMMAND Sts1CobcSwTests_Id)

    add_test_program(IdCounters)
    target_link_libraries(
        Sts1CobcSwTests_IdCounters PRIVATE Sts1CobcSw_RfProtocols Sts1CobcSwTests::CatchRodos
    )
    add_test(NAME IdCounters COMMAND Sts1CobcSwTests_IdCounters)

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
        PRIVATE Catch2::Catch2WithMain Sts1CobcSw_FramSections Sts1CobcSw_Serial Sts1CobcSw_Utility
    )
    catch_discover_tests(Sts1CobcSwTests_PersistentVariableInfo)

    add_test_program(PersistentVariables)
    target_link_libraries(
        Sts1CobcSwTests_PersistentVariables
        PRIVATE strong_type::strong_type Sts1CobcSw_Fram Sts1CobcSw_FramSections Sts1CobcSw_Serial
                Sts1CobcSw_Utility Sts1CobcSwTests::CatchRodos
    )
    add_test(NAME PersistentVariables COMMAND Sts1CobcSwTests_PersistentVariables)

    add_test_program(Reports)
    target_link_libraries(
        Sts1CobcSwTests_Reports
        PRIVATE strong_type::strong_type
                Sts1CobcSw_FileSystem
                Sts1CobcSw_Outcome
                Sts1CobcSw_RealTime
                Sts1CobcSw_RfProtocols
                Sts1CobcSw_Serial
                Sts1CobcSw_Telemetry
                Sts1CobcSw_Utility
                Sts1CobcSwTests::CatchRodos
                Sts1CobcSwTests::Utility
    )
    add_test(NAME Reports COMMAND Sts1CobcSwTests_Reports)

    add_test_program(Requests)
    target_link_libraries(
        Sts1CobcSwTests_Requests
        PRIVATE etl::etl
                strong_type::strong_type
                Sts1CobcSw_Edu
                Sts1CobcSw_Outcome
                Sts1CobcSw_RfProtocols
                Sts1CobcSw_Serial
                Sts1CobcSwTests::CatchRodos
                Sts1CobcSwTests::Utility
    )
    add_test(NAME Requests COMMAND Sts1CobcSwTests_Requests)

    add_test_program(Section)
    target_link_libraries(
        Sts1CobcSwTests_Section PRIVATE Catch2::Catch2WithMain strong_type::strong_type
                                        Sts1CobcSw_Fram Sts1CobcSw_FramSections
    )
    catch_discover_tests(Sts1CobcSwTests_Section)

    add_test_program(Serial)
    target_link_libraries(
        Sts1CobcSwTests_Serial PRIVATE Catch2::Catch2WithMain etl::etl Sts1CobcSw_Serial
    )
    catch_discover_tests(Sts1CobcSwTests_Serial)

    add_test_program(SingleBuffer)
    target_link_libraries(
        Sts1CobcSwTests_SingleBuffer
        PRIVATE strong_type::strong_type Sts1CobcSw_Outcome Sts1CobcSw_SingleBuffer
                Sts1CobcSw_Vocabulary Sts1CobcSwTests::CatchRodos
    )
    add_test(NAME SingleBuffer COMMAND Sts1CobcSwTests_SingleBuffer)

    add_test_program(SpacePacket)
    target_link_libraries(
        Sts1CobcSwTests_SpacePacket
        PRIVATE etl::etl Sts1CobcSw_Outcome Sts1CobcSw_RfProtocols Sts1CobcSw_Serial
                Sts1CobcSwTests::CatchRodos Sts1CobcSwTests::Utility
    )
    add_test(NAME SpacePacket COMMAND Sts1CobcSwTests_SpacePacket)

    add_test_program(Span)
    target_link_libraries(
        Sts1CobcSwTests_Span PRIVATE Catch2::Catch2WithMain etl::etl Sts1CobcSw_Utility
    )
    catch_discover_tests(Sts1CobcSwTests_Span)

    add_test_program(StringLiteral)
    target_link_libraries(
        Sts1CobcSwTests_StringLiteral PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Utility
    )
    catch_discover_tests(Sts1CobcSwTests_StringLiteral)

    add_test_program(SubsectionInfo)
    target_link_libraries(
        Sts1CobcSwTests_SubsectionInfo PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Fram
                                               Sts1CobcSw_FramSections Sts1CobcSw_Utility
    )
    catch_discover_tests(Sts1CobcSwTests_SubsectionInfo)

    add_test_program(Subsections)
    target_link_libraries(
        Sts1CobcSwTests_Subsections
        PRIVATE Catch2::Catch2WithMain strong_type::strong_type Sts1CobcSw_Fram
                Sts1CobcSw_FramSections Sts1CobcSw_Utility
    )
    catch_discover_tests(Sts1CobcSwTests_Subsections)

    add_test_program(TcSpacePacketSecondaryHeader)
    target_link_libraries(
        Sts1CobcSwTests_TcSpacePacketSecondaryHeader
        PRIVATE Sts1CobcSw_RfProtocols Sts1CobcSw_Serial Sts1CobcSwTests::CatchRodos
                Sts1CobcSwTests::Utility
    )
    add_test(NAME TcSpacePacketSecondaryHeader COMMAND Sts1CobcSwTests_TcSpacePacketSecondaryHeader)

    add_test_program(TcTransferFrame)
    target_link_libraries(
        Sts1CobcSwTests_TcTransferFrame
        PRIVATE Sts1CobcSw_Outcome Sts1CobcSw_RfProtocols Sts1CobcSw_Serial
                Sts1CobcSwTests::CatchRodos Sts1CobcSwTests::Utility
    )
    add_test(NAME TcTransferFrame COMMAND Sts1CobcSwTests_TcTransferFrame)

    add_test_program(TelemetryRecord)
    target_link_libraries(
        Sts1CobcSwTests_TelemetryRecord
        PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Edu Sts1CobcSw_Serial Sts1CobcSw_Telemetry
                Sts1CobcSw_Vocabulary
    )
    catch_discover_tests(Sts1CobcSwTests_TelemetryRecord)

    add_test_program(TmTransferFrame)
    target_link_libraries(
        Sts1CobcSwTests_TmTransferFrame
        PRIVATE etl::etl Sts1CobcSw_ChannelCoding Sts1CobcSw_RfProtocols Sts1CobcSw_Serial
                Sts1CobcSw_Utility Sts1CobcSwTests::CatchRodos
    )
    add_test(NAME TmTransferFrame COMMAND Sts1CobcSwTests_TmTransferFrame)

    add_test_program(UInt)
    target_link_libraries(Sts1CobcSwTests_UInt PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Serial)
    catch_discover_tests(Sts1CobcSwTests_UInt)
endif()

# ---- All COBC tests in one target ----

if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    get_property(
        all_test_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS
    )
    set(all_sources "")
    set(all_link_libraries "")
    foreach(target IN LISTS all_test_targets)
        if(target STREQUAL Sts1CobcSwTests_FirmwareManagement)
            # We must skip the FW management test because it erases everything after the first 128
            # kiB of the flash and AlmostAllUnitTests is > 128 kiB in debug mode
            continue()
        endif()
        get_target_property(sources ${target} SOURCES)
        list(APPEND all_sources ${sources})
        get_target_property(link_libraries ${target} LINK_LIBRARIES)
        list(APPEND all_link_libraries ${link_libraries})
    endforeach()
    list(REMOVE_DUPLICATES all_sources)
    list(REMOVE_DUPLICATES all_link_libraries)
    add_program(AlmostAllUnitTests ${all_sources})
    target_link_libraries(Sts1CobcSwTests_AlmostAllUnitTests PRIVATE ${all_link_libraries})
endif()

# ---- All unit tests ----

get_property(unit_test_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
add_custom_target(AllUnitTests)
add_dependencies(AllUnitTests ${unit_test_targets})
