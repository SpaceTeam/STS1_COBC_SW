#include <Sts1CobcSw/RodosTime/RodosTime.hpp>

#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>


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
