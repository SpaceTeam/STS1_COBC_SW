#pragma once


#include <Sts1CobcSw/Bootloader/UciUart.hpp>


#ifdef ENABLE_DEBUG_PRINT
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage, *variadic-macro-arguments)
    #define DEBUG_PRINT(fmt, ...) sts1cobcsw::uciuart::PrintF(fmt, ##__VA_ARGS__)
    #pragma GCC diagnostic pop
#else
    #define DEBUG_PRINT(fmt, ...)
#endif
