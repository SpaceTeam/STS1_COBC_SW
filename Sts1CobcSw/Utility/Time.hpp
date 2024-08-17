#pragma once


// TODO: Just including timemodel.h might be better here because then Utility should be able to be
// combined with Catch2 without defining two main()s.
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/arithmetic.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/ordered.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
using Duration = strong::type<std::int64_t,
                              struct DurationTag,
                              strong::equality,
                              strong::strongly_ordered,
                              strong::arithmetic>;
using RodosTime = strong::type<std::int64_t,
                               struct RodosTimeTag,
                               strong::affine_point<Duration>,
                               strong::equality,
                               strong::strongly_ordered>;

using RealTime =
    strong::type<std::int32_t, struct RealTimeTag, strong::equality, strong::strongly_ordered>;

//! Number of nanoseconds between 01.01.1970 and 01.01.2000
constexpr auto rodosUnixOffset = 946'684'800 * RODOS::SECONDS;

template<>
inline constexpr std::size_t serialSize<RealTime> =
    totalSerialSize<strong::underlying_type_t<RealTime>>;

// Todo: Change name
//! @brief Print UTC system time in human readable format.
auto PrintFormattedSystemUtc() -> void;

[[nodiscard]] auto UnixToRodosTime(std::int32_t unixTimeSeconds) -> std::int64_t;
[[nodiscard]] auto CurrentRealTime() -> RealTime;
[[nodiscard]] auto CurrentRodosTime() -> RodosTime;

template<std::endian endianness>
[[nodiscard]] inline auto SerializeTo(void * destination, RealTime const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] inline auto DeserializeFrom(void const * source, RealTime * data) -> void const *;
}


#include <Sts1CobcSw/Utility/Time.ipp>  // IWYU pragma: keep
