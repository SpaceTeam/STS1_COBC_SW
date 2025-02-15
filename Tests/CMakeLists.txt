# Parent project does not export its library target, so this CML implicitly depends on being added
# from it, i.e. the testing is done only from the build tree and is not feasible from an install
# location

project(Sts1CobcSwTests LANGUAGES CXX)

# ---- Find dependencies ----

find_package_and_notify(Catch2)
find_package_and_notify(etl)
find_rodos()

# ---- Libraries ----

add_subdirectory(CatchRodos EXCLUDE_FROM_ALL)
# Even though it is called "HardwareSetup", it can be build for both Linux and the COBC. On Linux it
# just does not do anything. This simplifies the CMake code for tests that can be built for both
# targets.
add_subdirectory(HardwareSetup EXCLUDE_FROM_ALL)

# ---- Tests ----

add_custom_target(AllTests)
add_dependencies(
    AllTests Sts1CobcSwTests_CatchRodos_Example Sts1CobcSwTests_CatchRodos_FailMessages
)

add_subdirectory(UnitTests EXCLUDE_FROM_ALL)
add_dependencies(AllTests AllUnitTests)
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    add_subdirectory(GoldenTests EXCLUDE_FROM_ALL)
    add_dependencies(AllTests AllGoldenTests)
elseif(CMAKE_SYSTEM_NAME STREQUAL Generic)
    add_subdirectory(ManualTests EXCLUDE_FROM_ALL)
    add_dependencies(AllTests AllManualTests)
endif()

# ---- End-of-file commands ----

add_folders(Tests)
