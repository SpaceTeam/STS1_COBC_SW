#pragma once


#include <rodos_no_using_namespace.h>


#ifdef ENABLE_DEBUG_PRINT
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage, *variadic-macro-arguments)
    #define DEBUG_PRINT(fmt, ...) RODOS::PRINTF(fmt, ##__VA_ARGS__)
    #pragma GCC diagnostic pop
#else
    #define DEBUG_PRINT(fmt, ...)
#endif

#ifdef ENABLE_DEBUG_PRINT_STACK_USAGE
    #define DEBUG_PRINT_STACK_USAGE() \
        DEBUG_PRINT( \
            "[%s#%i] max. stack usage = %5u B\n", __FILE_NAME__, __LINE__, getMaxStackUsage())
#else
    #define DEBUG_PRINT_STACK_USAGE()
#endif
