# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(CobcSwTests LANGUAGES CXX)

# ---- Tests ----

add_executable(CobcSw_Test Source/CobcSw_Test.cpp)
target_link_libraries(CobcSw_Test PRIVATE CobcSw_Lib)
target_compile_features(CobcSw_Test PRIVATE cxx_std_20)

add_test(NAME CobcSw_Test COMMAND CobcSw_Test)

# ---- End-of-file commands ----

add_folders(Test)
