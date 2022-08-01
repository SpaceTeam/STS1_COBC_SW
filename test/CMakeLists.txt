# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(CobcSwTests LANGUAGES CXX)

# ---- Tests ----

add_executable(CobcSw_test source/CobcSw_test.cpp)
target_link_libraries(CobcSw_test PRIVATE CobcSw_lib)
target_compile_features(CobcSw_test PRIVATE cxx_std_20)

add_test(NAME CobcSw_test COMMAND CobcSw_test)

# ---- End-of-file commands ----

add_folders(Test)
