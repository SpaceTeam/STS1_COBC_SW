if(CMAKE_CXX_CLANG_TIDY)
    find_program(CLANG_TIDY_CACHE_PATH NAMES "clang-tidy-cache")
    if(CLANG_TIDY_CACHE_PATH)
        message("Found clang-tidy-cache")
        message("clang-tidy-cache directory : $ENV{CTCACHE_DIR}")
        set(CLANG_TIDY_PATH "${CLANG_TIDY_CACHE_PATH};${CMAKE_CXX_CLANG_TIDY}"
            CACHE STRING "A combined command to run clang-tidy with caching wrapper"
        )
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_PATH}")
    else()
        message(WARNING "clang-tidy-cache not found in path")
    endif()
endif()
