#pragma once


#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/Utility/RealTime.hpp>


namespace sts1cobcsw
{
inline auto CurrentRealTime() -> RealTime
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


template<std::endian endianness>
inline auto SerializeTo(void * destination, RealTime const & data) -> void *
{
    return SerializeTo<endianness>(destination, value_of(data));
}


template<std::endian endianness>
inline auto DeserializeFrom(void const * source, RealTime * data) -> void const *
{
    source = DeserializeFrom<endianness>(source, &(value_of(*data)));
    return source;
}
}
