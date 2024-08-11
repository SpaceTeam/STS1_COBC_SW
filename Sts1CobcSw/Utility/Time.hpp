#pragma once


// TODO: Just including timemodel.h might be better here because then Utility should be able to be
// combined with Catch2 without defining two main()s.
#include <strong_type/affine_point.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw::utility
{
//! Number of nanoseconds between 01.01.1970 and 01.01.2000
constexpr auto rodosUnixOffset = 946'684'800 * RODOS::SECONDS;

using Duration = strong::type<std::int64_t, struct DurationTag>;
using RodosTime = strong::type<std::int64_t, struct RodosTimeTag, strong::affine_point<Duration>>;

using RealTimeDuration = strong::type<std::int32_t, struct RealTimeDurationTag>;
using RealTime =
    strong::type<std::int32_t, struct RealTimeTag, strong::affine_point<RealTimeDuration>>;

// Todo: Change name
//! @brief Print UTC system time in human readable format.
auto PrintFormattedSystemUtc() -> void;

[[nodiscard]] auto UnixToRodosTime(std::int32_t unixTimeSeconds) -> std::int64_t;
[[nodiscard]] auto GetUnixUtc() -> std::int32_t;
}


#include <Sts1CobcSw/Utility/Time.ipp>  // IWYU pragma: keep
