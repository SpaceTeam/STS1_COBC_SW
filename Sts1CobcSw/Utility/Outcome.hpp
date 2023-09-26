#pragma once


#define OUTCOME_DISABLE_EXECINFO
#define SYSTEM_ERROR2_NOT_POSIX
#define SYSTEM_ERROR2_FATAL(msg) RODOS::hwResetAndReboot()

#include <outcome-experimental.hpp>

#include <rodos_no_using_namespace.h>


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
            RODOS::hwResetAndReboot();
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
            RODOS::hwResetAndReboot();
        }
    }

    template<class Impl>
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr void wide_exception_check(Impl && self)
    {
        if(!base::_has_exception(std::forward<Impl>(self)))
        {
            RODOS::hwResetAndReboot();
        }
    }
};
