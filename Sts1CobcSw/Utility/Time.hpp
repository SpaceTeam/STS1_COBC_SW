#pragma once


#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
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


[[nodiscard]] auto ToRodosTime(RealTime realTime) -> RodosTime;
[[nodiscard]] auto ToRealTime(RodosTime rodosTime) -> RealTime;
[[nodiscard]] auto CurrentRealTime() -> RealTime;
[[nodiscard]] auto CurrentRodosTime() -> RodosTime;
auto SuspendUntil(RodosTime time) -> void;
auto SuspendFor(Duration duration) -> void;
}


#include <Sts1CobcSw/Utility/Time.ipp>  // IWYU pragma: keep
