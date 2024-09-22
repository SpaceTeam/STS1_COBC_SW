#pragma once


#include <Sts1CobcSw/Utility/RodosTime.hpp>


namespace sts1cobcsw
{
inline auto CurrentRodosTime() -> RodosTime
{
    return RodosTime(RODOS::NOW());
}


inline auto SuspendUntil(RodosTime time) -> void
{
    RODOS::AT(value_of(time));
}


inline auto SuspendFor(Duration duration) -> void
{
    RODOS::AT(RODOS::NOW() + value_of(duration));
}


template<std::endian endianness>
inline auto SerializeTo(void * destination, Duration const & data) -> void *
{
    return SerializeTo<endianness>(destination, value_of(data));
}


template<std::endian endianness>
inline auto DeserializeFrom(void const * source, Duration * data) -> void const *
{
    source = DeserializeFrom<endianness>(source, &(value_of(*data)));
    return source;
}
}
