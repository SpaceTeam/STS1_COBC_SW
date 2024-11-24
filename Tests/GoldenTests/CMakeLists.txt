include("${CMAKE_SOURCE_DIR}/cmake/add-golden-test.cmake")

add_golden_test(HelloWorld.test.cpp)
target_link_libraries(Sts1CobcSwTests_HelloWorld PRIVATE rodos::rodos)

add_golden_test(HelloDummy.test.cpp)
target_link_libraries(Sts1CobcSwTests_HelloDummy PRIVATE rodos::rodos etl::etl Sts1CobcSw_Dummy)

# --- All golden tests ---

get_property(
    all_golden_test_targets DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BUILDSYSTEM_TARGETS
)
add_custom_target(AllGoldenTests) # Must be defined after getting all hardware test targets
add_dependencies(AllGoldenTests ${all_golden_test_targets})
