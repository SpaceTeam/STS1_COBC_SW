#include <Sts1CobcSw/RfProtocols/Vocabulary.hpp>


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
auto SerializeTo(void * destination, Parameter const & parameter) -> void *
{
    destination = SerializeTo<endianness>(destination, parameter.parameterId);
    destination = SerializeTo<endianness>(destination, parameter.parameterValue);
    return destination;
}


template auto SerializeTo<std::endian::big>(void *, MessageTypeIdFields const &) -> void *;
template auto SerializeTo<std::endian::little>(void *, MessageTypeIdFields const &) -> void *;
template auto SerializeTo<std::endian::big>(void *, Parameter const &) -> void *;
template auto SerializeTo<std::endian::little>(void *, Parameter const &) -> void *;


template<std::endian endianness>
auto DeserializeFrom(void const * source, MessageTypeIdFields * fields) -> void const *
{
    source = DeserializeFrom<endianness>(source, &fields->serviceTypeId);
    source = DeserializeFrom<endianness>(source, &fields->messageSubtypeId);
    return source;
}


template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, Parameter * parameter) -> void const *
{
    source = DeserializeFrom<endianness>(source, &parameter->parameterId);
    source = DeserializeFrom<endianness>(source, &parameter->parameterValue);
    return source;
}


template auto DeserializeFrom<std::endian::big>(void const *, MessageTypeIdFields *)
    -> void const *;
template auto DeserializeFrom<std::endian::little>(void const *, MessageTypeIdFields *)
    -> void const *;
template auto DeserializeFrom<std::endian::big>(void const *, Parameter *) -> void const *;
template auto DeserializeFrom<std::endian::little>(void const *, Parameter *) -> void const *;
}
