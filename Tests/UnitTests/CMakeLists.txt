# --- Tests with Catch2 ---

# TODO: Implement something like add_test_program() which adds the suffix "Test" to the output name

add_program(Dummy Dummy.test.cpp)
target_link_libraries(Sts1CobcSwTests_Dummy PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Dummy)

add_program(FlatArray FlatArray.test.cpp)
target_link_libraries(Sts1CobcSwTests_FlatArray PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Utility)

add_program(Fram Fram.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Fram PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Periphery Sts1CobcSw_Serial
                                 Sts1CobcSw_Utility
)

add_program(LfsRam LfsRam.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_LfsRam PRIVATE Catch2::Catch2WithMain littlefs::littlefs Sts1CobcSw_FileSystem
)

add_program(MajorityVote MajorityVote.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_MajorityVote PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Utility
)

add_program(Outcome Outcome.test.cpp)
target_link_libraries(Sts1CobcSwTests_Outcome PRIVATE Catch2::Catch2WithMain)

add_program(PersistentVariableInfo PersistentVariableInfo.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_PersistentVariableInfo
    PRIVATE Catch2::Catch2WithMain Sts1CobcSw_FramSections Sts1CobcSw_Periphery Sts1CobcSw_Serial
            Sts1CobcSw_Utility
)

add_program(Section Section.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Section PRIVATE Catch2::Catch2WithMain Sts1CobcSw_FramSections
                                    Sts1CobcSw_Periphery
)

add_program(Serial Serial.test.cpp)
target_link_libraries(Sts1CobcSwTests_Serial PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Serial)

add_program(Span Span.test.cpp)
target_link_libraries(Sts1CobcSwTests_Span PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Utility)

add_program(StringLiteral StringLiteral.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_StringLiteral PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Utility
)

add_program(SubsectionInfo SubsectionInfo.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_SubsectionInfo PRIVATE Catch2::Catch2WithMain Sts1CobcSw_FramSections
                                           Sts1CobcSw_Periphery Sts1CobcSw_Utility
)

add_program(Subsections Subsections.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_Subsections PRIVATE Catch2::Catch2WithMain Sts1CobcSw_FramSections
                                        Sts1CobcSw_Periphery Sts1CobcSw_Utility
)

get_property(
    all_catch2_unit_test_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS
)
include(Catch)
foreach(target IN LISTS all_catch2_unit_test_targets)
    catch_discover_tests(${target})
endforeach()

# --- Tests with Rodos ---

add_program(EdacVariable EdacVariable.test.cpp UnitTestThread.cpp)
target_link_libraries(Sts1CobcSwTests_EdacVariable PRIVATE rodos::rodos Sts1CobcSw_Utility)
add_test(NAME EdacVariable COMMAND Sts1CobcSwTests_EdacVariable)
# Disable warnings about memcpy() of non-trivially copiable type EdacVariable<>
target_compile_options(Sts1CobcSwTests_EdacVariable PRIVATE -Wno-class-memaccess)

add_program(FramRingArray FramRingArray.test.cpp UnitTestThread.cpp)
target_link_libraries(
    Sts1CobcSwTests_FramRingArray PRIVATE rodos::rodos Sts1CobcSw_FramSections Sts1CobcSw_Periphery
                                          Sts1CobcSw_Serial
)
add_test(NAME FramRingArray COMMAND Sts1CobcSwTests_FramRingArray)

add_program(LfsWrapper LfsWrapper.test.cpp UnitTestThread.cpp)
target_link_libraries(
    Sts1CobcSwTests_LfsWrapper PRIVATE rodos::rodos littlefs::littlefs Sts1CobcSw_FileSystem
)
add_test(NAME LfsWrapper COMMAND Sts1CobcSwTests_FramRingArray)

add_program(PersistentVariables PersistentVariables.test.cpp UnitTestThread.cpp)
target_link_libraries(
    Sts1CobcSwTests_PersistentVariables
    PRIVATE rodos::rodos Sts1CobcSw_FramSections Sts1CobcSw_Periphery Sts1CobcSw_Serial
            Sts1CobcSw_Utility
)
add_test(NAME PersistentVariables COMMAND Sts1CobcSwTests_PersistentVariables)

# --- All unit tests ---

get_property(
    all_unit_test_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS
)
add_custom_target(AllUnitTests) # Must be defined after getting all unit test targets
add_dependencies(AllUnitTests ${all_unit_test_targets})
