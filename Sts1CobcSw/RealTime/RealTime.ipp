#pragma once


#include <Sts1CobcSw/RealTime/RealTime.hpp>

#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>

#include <strong_type/type.hpp>

#include <rodos/api/timemodel.h>


namespace sts1cobcsw
{
inline auto CurrentRealTime() -> RealTime
{
    return ToRealTime(CurrentRodosTime());
}


inline auto ToRodosTime(RealTime realTime) -> RodosTime
{
    return RodosTime(value_of(realTime) * RODOS::SECONDS)
         - persistentVariables.Load<"realTimeOffset">();
}


inline auto ToRealTime(RodosTime rodosTime) -> RealTime
{
    return RealTime(value_of(rodosTime + persistentVariables.Load<"realTimeOffset">())
                    / RODOS::SECONDS);
}
}
