#pragma once


#include <rodos_no_using_namespace.h>


#ifdef DEBUG
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage, *variadic-macro-arguments)
    #define DEBUG_PRINT(fmt, ...) RODOS::PRINTF(fmt, ##__VA_ARGS__)
    #pragma GCC diagnostic pop
#else
    #define DEBUG_PRINT(fmt, ...)
#endif
