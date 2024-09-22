#pragma once


#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>

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


[[nodiscard]] auto CurrentRodosTime() -> RodosTime;
auto SuspendUntil(RodosTime time) -> void;
auto SuspendFor(Duration duration) -> void;
}


#include <Sts1CobcSw/Utility/RodosTime.ipp>  // IWYU pragma: keep
