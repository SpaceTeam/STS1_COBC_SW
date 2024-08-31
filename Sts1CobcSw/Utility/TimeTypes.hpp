#pragma once

#include <Sts1CobcSw/Serial/Serial.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

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
using RealTime = strong::type<std::int32_t,
                              struct RealTimeTag,
                              strong::default_constructible,
                              strong::equality,
                              strong::strongly_ordered>;


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


#include "TimeTypes.ipp"  // IWYU pragma: keep
