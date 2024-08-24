#pragma once

#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
// TODO: Make all those single line functions inline and move them to the .ipp file
inline auto ToRodosTime(RealTime realTime) -> RodosTime
{
    return RodosTime(value_of(realTime) * RODOS::SECONDS) - internal::realTimeOffset;
}


inline auto ToRealTime(RodosTime rodosTime) -> RealTime
{
    return RealTime(value_of(rodosTime + internal::realTimeOffset) / RODOS::SECONDS);
}


inline auto CurrentRealTime() -> RealTime
{
    return ToRealTime(CurrentRodosTime());
}


inline auto CurrentRodosTime() -> RodosTime
{
    return RodosTime(RODOS::NOW());
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
