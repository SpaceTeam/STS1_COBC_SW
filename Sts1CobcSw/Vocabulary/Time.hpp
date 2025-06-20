#pragma once


#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <rodos/api/timemodel.h>

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
}
