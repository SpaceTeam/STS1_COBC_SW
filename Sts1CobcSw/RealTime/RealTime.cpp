#include <Sts1CobcSw/RealTime/RealTime.hpp>

#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <climits>
#include <utility>


namespace sts1cobcsw
{
auto UpdateRealTimeOffset(RealTime telecommandTimestamp, std::int32_t rxBaudRate) -> void
{
    // TODO: Get this from the CCSDS TC Space Data Link Protocol config once we have it
    static constexpr auto telecommandSize = 383;
    auto currentRodosTime = CurrentRodosTime();
    auto const transmissionDuration = telecommandSize * CHAR_BIT * 1000 / rxBaudRate * ms;
    auto offsetCorrection = persistentVariables.template Load<"realTimeOffsetCorrection">();
    auto newRealTimeOffset = RodosTime(value_of(telecommandTimestamp) * RODOS::SECONDS)
                           + transmissionDuration + offsetCorrection - currentRodosTime;
    persistentVariables.template Store<"realTimeOffset">(newRealTimeOffset);
}
}
