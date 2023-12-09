#pragma once

#ifdef DEBUG
    // NOLINTNEXTLINE(readability-identifier-naming, cppcoreguidelines-macro-usage,
    // clang-diagnostic-gnu-zero-variadic-macro-arguments)
    #define DebugPrint(fmt, ...) RODOS::PRINTF(fmt, ##__VA_ARGS__)
#else
    #define DebugPrint(fmt, ...)
#endif
