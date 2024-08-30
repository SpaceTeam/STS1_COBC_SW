#pragma once

#include <Sts1CobcSw/Utility/TimeTypes.hpp>

namespace sts1cobcsw
{
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
