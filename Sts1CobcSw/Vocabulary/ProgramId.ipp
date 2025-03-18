#pragma once


#include <Sts1CobcSw/Vocabulary/ProgramId.hpp>


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
    source = DeserializeFrom<endianness>(source, &(value_of(*data)));
    return source;
}
}
