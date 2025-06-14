#pragma once


#include <Sts1CobcSw/Vocabulary/MessageTypeIdFields.hpp>


namespace sts1cobcsw
{
template<std::endian endianness>
auto SerializeTo(void * destination, MessageTypeIdFields const & fields) -> void *
{
    destination = SerializeTo<endianness>(destination, fields.serviceTypeId);
    destination = SerializeTo<endianness>(destination, fields.messageSubtypeId);
    return destination;
}


template<std::endian endianness>
auto DeserializeFrom(void const * source, MessageTypeIdFields * fields) -> void const *
{
    source = DeserializeFrom<endianness>(source, &fields->serviceTypeId);
    source = DeserializeFrom<endianness>(source, &fields->messageSubtypeId);
    return source;
}
}
