#pragma once


// TODO: Just including timemodel.h might be better here because then Utility should be able to be
// combined with Catch2 without defining two main()s.
#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw::utility
{
//! Number of nanoseconds between 01.01.1970 and 01.01.2000
constexpr auto rodosUnixOffset = 946'684'800 * RODOS::SECONDS;


//! @brief Print UTC system time in human readable format.
auto PrintTime() -> void;


//! @brief Given a time in seconds since 01.01.1970, return a time in nanoseconds since 01.01.2000.
[[nodiscard]] inline auto UnixToRodosTime(std::int32_t const unixTimeSeconds)
{
    return static_cast<std::int64_t>(unixTimeSeconds) * RODOS::SECONDS - rodosUnixOffset;
}


[[nodiscard]] inline auto GetUnixUtc()
{
    auto unixUtc = (RODOS::sysTime.getUTC() + rodosUnixOffset) / RODOS::SECONDS;
    return static_cast<std::int32_t>(unixUtc);
}
}
