#pragma once


#include <Sts1CobcSw/Time/RealTime.hpp>

#include <rodos_no_using_namespace.h>

#include <cinttypes>


#ifdef ENABLE_DEBUG_PRINT
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage, *variadic-macro-arguments)
    #define DEBUG_PRINT(fmt, ...) RODOS::PRINTF(fmt, ##__VA_ARGS__)
    #pragma GCC diagnostic pop
#else
    #define DEBUG_PRINT(fmt, ...)
#endif

#define DEBUG_PRINT_REAL_TIME() DEBUG_PRINT("Real time: %" PRIi32 "\n", value_of(CurrentRealTime()))
