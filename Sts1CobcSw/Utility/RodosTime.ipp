#pragma once


#include <Sts1CobcSw/Utility/RodosTime.hpp>

#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
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


inline auto BusyWaitUntil(RodosTime time) -> void
{
    RODOS::BUSY_WAITING_UNTIL(value_of(time));
}


inline auto BusyWaitFor(Duration duration) -> void
{
    RODOS::BUSY_WAITING_UNTIL(RODOS::NOW() + value_of(duration));
}
}
