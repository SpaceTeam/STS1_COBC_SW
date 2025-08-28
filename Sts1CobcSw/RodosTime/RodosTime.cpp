#include <Sts1CobcSw/RodosTime/RodosTime.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <compare>
#include <utility>


namespace sts1cobcsw
{
auto SuspendUntilResumedOr(RodosTime time) -> Result<void>
{
    SuspendUntil(time);
    if(CurrentRodosTime() >= time)
    {
        return ErrorCode::timeout;
    }
    return outcome_v2::success();
}


auto SuspendUntilResumed(Duration timeout) -> Result<void>
{
    return SuspendUntilResumedOr(CurrentRodosTime() + timeout);
}
}
