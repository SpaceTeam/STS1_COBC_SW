#pragma once

#ifndef BUILD_BOOTLOADER
    #include <Sts1CobcSw/RodosTime/RodosTime.hpp>

    #include <rodos_no_using_namespace.h>


    #ifdef REDIRECT_RF_OVER_UCI
inline auto uciUartSemaphore = RODOS::Semaphore{};

        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic"
        // NOLINTBEGIN(cppcoreguidelines-macro-usage, *variadic-macro-arguments)
        #define PROTECTED_PRINTF(fmt, ...) \
            do /* NOLINT(*avoid-do-while) */ \
            { \
                auto uciUartProtector = RODOS::ScopeProtector(&uciUartSemaphore); \
                RODOS::PRINTF(fmt, ##__VA_ARGS__); \
                BusyWaitFor(2 * ms); \
            } while(false)
        // NOLINTEND(cppcoreguidelines-macro-usage, *variadic-macro-arguments)
        #pragma GCC diagnostic pop
    #else
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic"
        // NOLINTNEXTLINE(cppcoreguidelines-macro-usage, *variadic-macro-arguments)
        #define PROTECTED_PRINTF(fmt, ...) RODOS::PRINTF(fmt, ##__VA_ARGS__)
        #pragma GCC diagnostic pop
    #endif

    #ifdef ENABLE_DEBUG_PRINT
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic"
        // NOLINTNEXTLINE(cppcoreguidelines-macro-usage, *variadic-macro-arguments)
        #define DEBUG_PRINT(fmt, ...) PROTECTED_PRINTF(fmt, ##__VA_ARGS__)
        #pragma GCC diagnostic pop
    #else
        #define DEBUG_PRINT(fmt, ...)
    #endif

    #ifdef ENABLE_DEBUG_PRINT_STACK_USAGE
        #define DEBUG_PRINT_STACK_USAGE() \
            DEBUG_PRINT("[%s#%i] max. stack usage = %5u B\n", \
                        __FILE_NAME__, \
                        __LINE__, \
                        RODOS::Thread::getCurrentThread()->getMaxStackUsage()); \
            BusyWaitFor(2 * ms)
    #else
        #define DEBUG_PRINT_STACK_USAGE()
    #endif

    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage, *variadic-macro-arguments)
    #define PRINT_STACK_USAGE() \
        PROTECTED_PRINTF("[%s#%i] max. stack usage = %5u B\n", \
                         __FILE_NAME__, \
                         __LINE__, \
                         RODOS::Thread::getCurrentThread()->getMaxStackUsage()); \
        BusyWaitFor(2 * ms)
#endif
