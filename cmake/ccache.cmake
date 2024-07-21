# Ccache is a compiler cache. It speeds up recompilation by caching previous compilations and
# detecting when the same compilation is being done again. This is especially useful when switching
# between branches that require a full rebuild.
if(PROJECT_IS_TOP_LEVEL)
    option(USE_CCACHE "Use ccache to speed up recompilation" ON)
    if(USE_CCACHE)
        find_program(CCACHE_PROGRAM ccache)
        if(CCACHE_PROGRAM)
            message("Found ccache: ${CCACHE_PROGRAM}")
            set(ccacheEnv
                # If most of the sources come from this project, CMAKE_SOURCE_DIR is a good
                # CCACHE_BASEDIR. Otherwise, you can try setting it to CMAKE_BINARY_DIR. Compare
                # ccache's statistics with ccache -s [-v[v]] between builds to see if it's helping.
                CCACHE_BASEDIR=${CMAKE_SOURCE_DIR}
                # Use the following sloppiness settings when using precompiled headers
                # CCACHE_SLOPPINESS=pch_defines,time_macros
            )
            set(CMAKE_CXX_COMPILER_LAUNCHER ${CMAKE_COMMAND} -E env ${ccacheEnv} ${CCACHE_PROGRAM})
        endif()
    endif()
endif()
