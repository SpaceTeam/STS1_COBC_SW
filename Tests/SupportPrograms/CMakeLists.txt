if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    add_program(ListEduFiles)
    target_sources(Sts1CobcSwTests_ListEduFiles PRIVATE ListEduFiles.cpp)
    target_link_libraries(
        Sts1CobcSwTests_ListEduFiles
        PRIVATE rodos::rodos Sts1CobcSw_Edu Sts1CobcSw_FileSystem Sts1CobcSw_Fram
                Sts1CobcSw_Outcome Sts1CobcSwTests::HardwareSetup
    )
endif()

get_property(
    support_program_targets DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BUILDSYSTEM_TARGETS
)
add_custom_target(AllSupportPrograms)
if(support_program_targets)
    add_dependencies(AllSupportPrograms ${support_program_targets})
endif()
