#pragma once

#ifndef USE_NO_RODOS
    #include <rodos_no_using_namespace.h>
#else
    #include <iostream>
#endif


#if defined(SYSTEM_ERROR2_NOT_POSIX)
    // NOLINTNEXTLINE(*macro-usage)
    #ifdef USE_NO_RODOS
        #define SYSTEM_ERROR2_FATAL(msg)
    #else
        #define SYSTEM_ERROR2_FATAL(msg) RODOS::hwResetAndReboot()
    #endif
#endif

#include <outcome-experimental.hpp>  // IWYU pragma: export

#ifdef USE_NO_RODOS
// First define a policy
struct RebootPolicy : outcome_v2::experimental::policy::base
{
    template<class Impl>
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr void wide_value_check(Impl && self)
    {
        if(!base::_has_value(std::forward<Impl>(self)))
        {
            std::cout << "Error in wide_value_check(): _has_value() returned false. "
                      << "Calling std::abort().\n ";
            std::abort();
        }
    }

    template<class Impl>
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr void wide_error_check(Impl && self)
    {
        if(!base::_has_error(std::forward<Impl>(self)))
        {
            std::cout << "Error in wide_error_check(): _has_error() returned false. "
                      << "Calling std::abort().\n ";
            std::abort();
        }
    }

    template<class Impl>
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr void wide_exception_check(Impl && self)
    {
        if(!base::_has_exception(std::forward<Impl>(self)))
        {
            std::cout << "Error in wide_exception_check(): _has_exception() returned false. "
                      << "Calling std::abort().\n ";
            std::abort();
        }
    }
};
#else
struct RebootPolicy : outcome_v2::experimental::policy::base
{
    template<class Impl>
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr void wide_value_check(Impl && self)
    {
        //! Call RODOS::hwResetAndReboot() whenever .value() is called on an object that does not
        //! contain a value
        if(!base::_has_value(std::forward<Impl>(self)))
        {
    #ifndef USE_NO_RODOS
            RODOS::PRINTF(
                "Error: The value is not present. Performing hardware reset and reboot.\n");
            RODOS::hwResetAndReboot();
    #endif
        }
    }

    template<class Impl>
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr void wide_error_check(Impl && self)
    {
        //! Call RODOS::hwResetAndReboot() whenever .error() is called on an object that does not
        //! contain an error
        if(!base::_has_error(std::forward<Impl>(self)))
        {
    #ifndef USE_NO_RODOS
            RODOS::PRINTF(
                "Error: The error is not present. Performing hardware reset and reboot.\n");
            RODOS::hwResetAndReboot();
    #endif
        }
    }

    template<class Impl>
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr void wide_exception_check(Impl && self)
    {
        if(!base::_has_exception(std::forward<Impl>(self)))
        {
    #ifndef USE_NO_RODOS
            RODOS::PRINTF(
                "Error: The exception is not present. Performing hardware reset and reboot.\n");
            RODOS::hwResetAndReboot();
    #endif
        }
    }
};
#endif
