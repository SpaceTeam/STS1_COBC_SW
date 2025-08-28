#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>


namespace sts1cobcsw
{
[[nodiscard]] auto CurrentRodosTime() -> RodosTime;
auto SuspendUntil(RodosTime time) -> void;
auto SuspendFor(Duration duration) -> void;
auto SuspendUntilResumedOr(RodosTime time) -> Result<void>;
auto SuspendUntilResumed(Duration timeout) -> Result<void>;
auto BusyWaitUntil(RodosTime time) -> void;
auto BusyWaitFor(Duration duration) -> void;
}


#include <Sts1CobcSw/RodosTime/RodosTime.ipp>  // IWYU pragma: keep
