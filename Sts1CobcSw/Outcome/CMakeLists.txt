target_link_libraries(Sts1CobcSw_Outcome INTERFACE rodos::without_main_on_linux)

# This is an ugly hack to get the right include directories for Outcome because it is not properly
# installed with CMake
get_target_property(rodos_include_dirs rodos::rodos INTERFACE_INCLUDE_DIRECTORIES)
list(GET rodos_include_dirs 0 rodos_include_dir)
string(REPLACE "/rodos" ";" parts "${rodos_include_dir}")
list(GET parts 0 outcome_include_dir)
message("Using the following include directory for Outcome: ${outcome_include_dir}")
target_include_directories(Sts1CobcSw_Outcome SYSTEM INTERFACE "${outcome_include_dir}")
