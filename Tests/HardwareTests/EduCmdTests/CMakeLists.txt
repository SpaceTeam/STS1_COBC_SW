add_program(UpdateTime UpdateTime.test.cpp)
target_link_libraries(Sts1CobcSwTests_UpdateTime PUBLIC rodos::rodos type_safe Sts1CobcSw_Periphery)

add_program(ExecuteProgram ExecuteProgram.test.cpp)
target_link_libraries(Sts1CobcSwTests_ExecuteProgram PUBLIC rodos::rodos type_safe Sts1CobcSw_Periphery Sts1CobcSw_Util)

add_program(GetStatus GetStatus.test.cpp)
target_link_libraries(Sts1CobcSwTests_GetStatus PUBLIC rodos::rodos type_safe Sts1CobcSw_Periphery)

get_property(
    edu_cmd_targets
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY BUILDSYSTEM_TARGETS
)
add_custom_target(EduCommandTests) # Must be defined after getting all edu test targets
add_dependencies(EduCommandTests ${edu_cmd_targets})
