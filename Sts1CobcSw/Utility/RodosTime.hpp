#pragma once


#include <Sts1CobcSw/Vocabulary/TimeTypes.hpp>


namespace sts1cobcsw
{
[[nodiscard]] auto CurrentRodosTime() -> RodosTime;
auto SuspendUntil(RodosTime time) -> void;
auto SuspendFor(Duration duration) -> void;
auto BusyWaitUntil(RodosTime time) -> void;
auto BusyWaitFor(Duration duration) -> void;
}


#include <Sts1CobcSw/Utility/RodosTime.ipp>  // IWYU pragma: keep
