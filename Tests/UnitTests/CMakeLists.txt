add_program(Dummy Dummy.test.cpp)
target_link_libraries(Sts1CobcSwTests_Dummy PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Dummy)

add_program(Serialize Serialize.test.cpp)
target_link_libraries(Sts1CobcSwTests_Serialize PRIVATE Catch2::Catch2WithMain Sts1CobcSw_Serialize)

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
