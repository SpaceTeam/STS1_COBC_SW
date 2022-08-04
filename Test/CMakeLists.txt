# Parent project does not export its library target, so this CML implicitly depends on being added
# from it, i.e. the testing is done only from the build tree and is not feasible from an install
# location

project(CobcSwTests LANGUAGES CXX)

# ---- Tests ----

add_executable(CobcSw_Test)
add_test(NAME CobcSw_Test COMMAND CobcSw_Test)

add_subdirectory(Source)

# ---- End-of-file commands ----

add_folders(Test)
