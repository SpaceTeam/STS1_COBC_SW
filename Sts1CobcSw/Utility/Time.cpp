#include <Sts1CobcSw/Utility/Time.hpp>

#include <cinttypes>


namespace sts1cobcsw
{
auto realTimeOffset = Duration(0);


// TODO: Make all those single line functions inline and move them to the .ipp file
auto ToRodosTime(RealTime realTime) -> RodosTime
{
    return RodosTime(value_of(realTime) * RODOS::SECONDS) - realTimeOffset;
}


auto ToRealTime(RodosTime rodosTime) -> RealTime
{
    return RealTime(value_of(rodosTime + realTimeOffset) / RODOS::SECONDS);
}


auto CurrentRealTime() -> RealTime
{
    return ToRealTime(CurrentRodosTime());
}


auto CurrentRodosTime() -> RodosTime
{
    return RodosTime(RODOS::NOW());
}
}
