#pragma once


#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <rodos_no_using_namespace.h>


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
