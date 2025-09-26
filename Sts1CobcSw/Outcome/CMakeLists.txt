target_link_libraries(Sts1CobcSw_Outcome INTERFACE littlefs::littlefs)
if(NOT BUILD_BOOTLOADER)
    target_link_libraries(Sts1CobcSw_Outcome INTERFACE rodos::rodos)
endif()

# This is an ugly hack to get the right include directories for Outcome because it is not properly
# installed with CMake
get_target_property(littlefs_include_dirs littlefs::littlefs INTERFACE_INCLUDE_DIRECTORIES)
list(GET littlefs_include_dirs 0 littlefs_include_dir)
string(REPLACE "/littlefs" ";" parts "${littlefs_include_dir}")
list(GET parts 0 outcome_include_dir)
message("Using the following include directory for Outcome: ${outcome_include_dir}")
target_include_directories(Sts1CobcSw_Outcome SYSTEM INTERFACE "${outcome_include_dir}")
