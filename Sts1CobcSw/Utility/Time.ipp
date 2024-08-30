#pragma once

#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
inline auto ToRodosTime(RealTime realTime) -> RodosTime
{
    return RodosTime(value_of(realTime) * RODOS::SECONDS) - internal::realTimeOffset;
}


inline auto ToRealTime(RodosTime rodosTime) -> RealTime
{
    return RealTime(value_of(rodosTime + internal::realTimeOffset) / RODOS::SECONDS);
}


inline auto CurrentRealTime() -> RealTime
{
    return ToRealTime(CurrentRodosTime());
}


inline auto CurrentRodosTime() -> RodosTime
{
    return RodosTime(RODOS::NOW());
}


inline auto SuspendUntil(RodosTime time) -> void
{
    RODOS::AT(value_of(time));
}


inline auto SuspendFor(Duration duration) -> void
{
    RODOS::AT(RODOS::NOW() + value_of(duration));
}
}
