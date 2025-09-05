#pragma once


#include <Sts1CobcSw/Bootloader/UciUart.hpp>


#ifdef ENABLE_DEBUG_PRINT
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage, *variadic-macro-arguments)
    #define DEBUG_PRINT(fmt, ...) sts1cobcsw::uciuart::PrintF(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...)
#endif
