add_program(EduCommands EduCommands.test.cpp)
target_link_libraries(
    Sts1CobcSwTests_EduCommands PUBLIC rodos::rodos Sts1CobcSw_Utility Sts1CobcSw_Serial
                                       Sts1CobcSw_Periphery Sts1CobcSw_Hal
)

get_property(
    edu_command_test_targets
    DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    PROPERTY BUILDSYSTEM_TARGETS
)

add_custom_target(AllEduCommandTests) # Must be defined after getting all edu test targets
add_dependencies(AllEduCommandTests ${edu_command_test_targets})
