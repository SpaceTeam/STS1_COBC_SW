if(CMAKE_SYSTEM_NAME STREQUAL Generic)
    add_program(FramExplorer)
    target_sources(Sts1CobcSwTests_FramExplorer PRIVATE FramExplorer.cpp)
    target_link_libraries(
        Sts1CobcSwTests_FramExplorer
        PRIVATE etl::etl
                rodos::rodos
                strong_type::strong_type
                Sts1CobcSw_FirmwareManagement
                Sts1CobcSw_Fram
                Sts1CobcSw_FramSections
                Sts1CobcSw_Hal
                Sts1CobcSw_Outcome
                Sts1CobcSw_Serial
                Sts1CobcSw_Utility
                Sts1CobcSw_Vocabulary
                Sts1CobcSwTests::HardwareSetup
    )
endif()

get_property(
    support_program_targets DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY BUILDSYSTEM_TARGETS
)
add_custom_target(AllSupportPrograms)
if(support_program_targets)
    add_dependencies(AllSupportPrograms ${support_program_targets})
endif()
