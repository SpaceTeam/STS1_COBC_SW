#pragma once


#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/RealTime/RealTime.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>

#include <strong_type/type.hpp>

#include <rodos/api/timemodel.h>


namespace sts1cobcsw
{
Enline auto CurrentRealTime() -> RealTime
{
    return ToRealTime(CurrentRodosTime());
}


inline auto ToRodosTime(RealTime realTime) -> RodosTime
{
    return RodosTime(value_of(realTime) * RODOS::SECONDS)
         - persistentVariables.template Load<"realTimeOffset">();
}

inline auto ToRealTime(RodosTime rodosTime) -> RealTime
{
    return RealTime(value_of(rodosTime + persistentVariables.template Load<"realTimeOffset">())
                    / RODOS::SECONDS);
}


auto UpdateRealTimeOffset(RealTime teleTimeStamp, std::int32_t rxBaudRate) -> void
{
    static constexpr auto teleCommandSizeBytes = 383;
    static constexpr auto bitsPerByte = 8;

    auto transmissionTimeSeconds =
        static_cast<double>(teleCommandSizeBytes * bitsPerByte) / static_cast<double>(rxBaudRate);
    auto transmissionDuration = Duration(transmissionTimeSeconds * RODOS::SECONDS);

    auto newRealTimeOffset =
        teleTimeStamp * RODOS::SECONDS + transmissionDuration - CurrentRodosTime;

    persistentVariables.template Store<"realTimeOffset">(newRealTimeOffset);
}

}
