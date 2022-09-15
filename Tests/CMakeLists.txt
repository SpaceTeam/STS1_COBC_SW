# Parent project does not export its library target, so this CML implicitly depends on being added
# from it, i.e. the testing is done only from the build tree and is not feasible from an install
# location

# Only include tests when building for Linux
if(NOT (CMAKE_SYSTEM_NAME STREQUAL Linux))
    return()
endif()

project(Sts1CobcSwTests LANGUAGES CXX)

# ---- Find libraries ----

find_package_and_notify(Catch2)
find_package_and_notify(etl)
find_package_and_notify(type_safe)
find_rodos()

# ---- Tests ----

add_subdirectory(UnitTests EXCLUDE_FROM_ALL)
add_subdirectory(GoldenTests EXCLUDE_FROM_ALL)

add_custom_target(AllTests)
add_dependencies(AllTests AllUnitTests AllGoldenTests)

# ---- End-of-file commands ----

add_folders(Tests)
