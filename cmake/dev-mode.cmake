include(cmake/folders.cmake)

if(NOT BUILD_BOOTLOADER)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(Tests)
    endif()

    option(BUILD_MCSS_DOCS "Build documentation using Doxygen and m.css" OFF)
    if(BUILD_MCSS_DOCS)
        include(cmake/docs.cmake)
    endif()

    option(ENABLE_COVERAGE "Enable coverage support separate from CTest's" OFF)
    if(ENABLE_COVERAGE)
        include(cmake/coverage.cmake)
    endif()
endif()

include(cmake/format-targets.cmake)
include(cmake/spell-targets.cmake)

add_folders(Project)
