add_program(ExecuteProgram ExecuteProgram.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_ExecuteProgram PUBLIC rodos::rodos Sts1CobcSw_Periphery Sts1CobcSw_Utility
)

add_program(GetStatus GetStatus.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_GetStatus PUBLIC rodos::rodos Sts1CobcSw_Periphery Sts1CobcSw_Utility
)

add_program(UpdateTime UpdateTime.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_UpdateTime PUBLIC rodos::rodos Sts1CobcSw_Periphery Sts1CobcSw_Utility
)

get_property(
    edu_command_test_targets
    DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    PROPERTY BUILDSYSTEM_TARGETS
)

add_custom_target(AllEduCommandTests) # Must be defined after getting all edu test targets
add_dependencies(AllEduCommandTests ${edu_command_test_targets})
