#include <Sts1CobcSw/RealTime/RealTime.hpp>

#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <utility>


namespace sts1cobcsw
{
auto UpdateRealTimeOffset(RealTime realTime) -> void
{
    auto currentRodosTime = CurrentRodosTime();
    auto newRealTimeOffset = RodosTime(value_of(realTime) * RODOS::SECONDS) - currentRodosTime;
    persistentVariables.Store<"realTimeOffset">(newRealTimeOffset);
}
}
