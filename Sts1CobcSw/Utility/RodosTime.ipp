#pragma once


#include <Sts1CobcSw/Utility/RodosTime.hpp>


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
}
