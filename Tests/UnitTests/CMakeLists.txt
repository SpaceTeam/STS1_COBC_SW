# TODO: Implement something like add_test_program() which adds the suffix "Test" to the output name
add_program(Dummy Dummy.test.cpp)
target_link_libraries(Sts1CobcSwTests_Dummy PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Dummy)

add_program(Outcome Outcome.test.cpp)
target_link_libraries(Sts1CobcSwTests_Outcome PRIVATE Catch2::Catch2WithMain)

# TODO: Fix the problem with two main()s, one from Rodos one from Catch, somehow
if(FALSE)
    add_program(PersistentState PersistentState.test.cpp)
    target_link_libraries(
        Sts1CobcSwTests_PersistentState PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Periphery
    )
endif()

add_program(Serial Serial.test.cpp)
target_link_libraries(Sts1CobcSwTests_Serial PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Serial)

add_program(Span Span.test.cpp)
target_link_libraries(Sts1CobcSwTests_Span PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Utility)

get_property(
    all_unit_test_targets
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY BUILDSYSTEM_TARGETS
)
add_custom_target(AllUnitTests) # Must be defined after getting all unit test targets
add_dependencies(AllUnitTests ${all_unit_test_targets})

include(Catch)
foreach(target IN LISTS all_unit_test_targets)
    catch_discover_tests(${target})
endforeach()
