#pragma once


#include <Sts1CobcSw/ProgramId/ProgramId.hpp>


namespace sts1cobcsw
{
template<std::endian endianness>
inline auto SerializeTo(void * destination, ProgramId const & data) -> void *
{
    return SerializeTo<endianness>(destination, data.get());
}


template<std::endian endianness>
inline auto DeserializeFrom(void const * source, ProgramId * data) -> void const *
{
    return DeserializeFrom<endianness>(source, &(data->get()));
}
}
