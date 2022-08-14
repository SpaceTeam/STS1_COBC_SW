# Parent project does not export its library target, so this CML implicitly depends on being added
# from it, i.e. the testing is done only from the build tree and is not feasible from an install
# location

# Only include tests when building for Linux
if(NOT (CMAKE_SYSTEM_NAME STREQUAL Linux))
    return()
endif()

project(CobcSwTests LANGUAGES CXX)

# ---- Find libraries ----

# find_package_and_notify(Catch2)
find_rodos()
find_package_and_notify(etl)

# ---- Tests ----

add_executable(CobcSw_Test)
add_test(NAME CobcSw_Test COMMAND CobcSw_Test)

add_subdirectory(Source)

# ---- End-of-file commands ----

add_folders(Tests)
