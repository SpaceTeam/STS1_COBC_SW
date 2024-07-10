#pragma once


#include <Sts1CobcSw/ProgramId/ProgramId.hpp>


namespace sts1cobcsw
{
template<std::endian endianness>
inline auto SerializeTo(void * destination, ProgramId const & data) -> void *
{
    return SerializeTo<endianness>(destination, value_of(data));
}


template<std::endian endianness>
inline auto DeserializeFrom(void const * source, ProgramId * data) -> void const *
{
    std::uint16_t underlyingValue;
    source = DeserializeFrom<endianness>(source, &underlyingValue);
    *data = ProgramId(underlyingValue);
    return source;
}
}
