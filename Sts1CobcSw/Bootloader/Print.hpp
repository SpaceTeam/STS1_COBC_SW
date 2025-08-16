#pragma once


namespace sts1cobcsw
{
[[gnu::format(printf, 1, 2)]] auto PrintF(char const * format, ...) -> void;
}


#ifdef ENABLE_DEBUG_PRINT
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage, *variadic-macro-arguments)
    #define DEBUG_PRINT(fmt, ...) sts1cobcsw::PrintF(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...)
#endif
