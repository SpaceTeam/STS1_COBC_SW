#include <Sts1CobcSw/RodosTime/RodosTime.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <compare>
#include <utility>


namespace sts1cobcsw
{
auto SuspendUntilResumed(Duration timeout) -> Result<void>
{
    auto suspendTime = CurrentRodosTime();
    SuspendFor(timeout);
    if(CurrentRodosTime() >= suspendTime + timeout)
    {
        return ErrorCode::timeout;
    }
    return outcome_v2::success();
}
}
