#pragma once


#include <Sts1CobcSw/Serial/Serial.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <bit>
#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
using Duration = strong::type<std::int64_t,
                              struct DurationTag,
                              strong::default_constructible,  // For RODOS::CommBuffer
                              strong::difference>;
using RodosTime = strong::type<std::int64_t,
                               struct RodosTimeTag,
                               strong::affine_point<Duration>,
                               strong::equality,
                               strong::strongly_ordered>;
using RealTime =
    strong::type<std::int32_t, struct RealTimeTag, strong::equality, strong::strongly_ordered>;


constexpr auto seconds = Duration(RODOS::SECONDS);
constexpr auto milliseconds = Duration(RODOS::MILLISECONDS);
constexpr auto microseconds = Duration(RODOS::MICROSECONDS);
constexpr auto nanoseconds = Duration(RODOS::NANOSECONDS);
constexpr auto minutes = Duration(RODOS::MINUTES);
constexpr auto hours = Duration(RODOS::HOURS);
constexpr auto days = Duration(RODOS::DAYS);
constexpr auto weeks = Duration(RODOS::WEEKS);
// NOLINTBEGIN(readability-identifier-length)
constexpr auto s = seconds;
constexpr auto ms = milliseconds;
constexpr auto us = microseconds;
constexpr auto ns = nanoseconds;
constexpr auto min = minutes;
constexpr auto h = hours;
constexpr auto d = days;
constexpr auto w = weeks;
// NOLINTEND(readability-identifier-length)
constexpr auto endOfTime = RodosTime(RODOS::END_OF_TIME);

template<>
inline constexpr std::size_t serialSize<RealTime> =
    totalSerialSize<strong::underlying_type_t<RealTime>>;


[[nodiscard]] auto ToRodosTime(RealTime realTime) -> RodosTime;
[[nodiscard]] auto ToRealTime(RodosTime rodosTime) -> RealTime;
[[nodiscard]] auto CurrentRealTime() -> RealTime;
[[nodiscard]] auto CurrentRodosTime() -> RodosTime;
auto SuspendUntil(RodosTime time) -> void;
auto SuspendFor(Duration duration) -> void;

template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, RealTime const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, RealTime * data) -> void const *;


// TODO: Replace this with a persistent variable
namespace internal
{
extern Duration realTimeOffset;
}
}


#include <Sts1CobcSw/Utility/Time.ipp>  // IWYU pragma: keep
