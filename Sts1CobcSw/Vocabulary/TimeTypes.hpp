#pragma once


#include <Sts1CobcSw/Serial/Serial.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <rodos/api/timemodel.h>

#include <bit>
#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
using Duration = strong::
    type<std::int64_t, struct DurationTag, strong::difference, strong::default_constructible>;
using RodosTime = strong::type<std::int64_t,
                               struct RodosTimeTag,
                               strong::affine_point<Duration>,
                               strong::default_constructible,
                               strong::equality,
                               strong::strongly_ordered>;
using RealTime = strong::type<std::int32_t,
                              struct RealTimeTag,
                              strong::default_constructible,
                              strong::equality,
                              strong::strongly_ordered>;


// NOLINTBEGIN(readability-identifier-length)
constexpr auto s = Duration(RODOS::SECONDS);
constexpr auto ms = Duration(RODOS::MILLISECONDS);
constexpr auto us = Duration(RODOS::MICROSECONDS);
constexpr auto ns = Duration(RODOS::NANOSECONDS);
constexpr auto min = Duration(RODOS::MINUTES);
constexpr auto h = Duration(RODOS::HOURS);
// NOLINTEND(readability-identifier-length)
constexpr auto days = Duration(RODOS::DAYS);
constexpr auto weeks = Duration(RODOS::WEEKS);
constexpr auto endOfTime = RodosTime(RODOS::END_OF_TIME);


template<>
inline constexpr std::size_t serialSize<RealTime> =
    totalSerialSize<strong::underlying_type_t<RealTime>>;
template<>
inline constexpr std::size_t serialSize<Duration> =
    totalSerialSize<strong::underlying_type_t<Duration>>;


template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, RealTime const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, RealTime * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, Duration const & data) -> void *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, Duration * data) -> void const *;
}


#include <Sts1CobcSw/Vocabulary/TimeTypes.ipp>  // IWYU pragma: keep
