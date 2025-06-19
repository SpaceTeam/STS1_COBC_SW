#include <Sts1CobcSw/RfProtocols/Vocabulary.hpp>


namespace sts1cobcsw
{
template<std::endian endianness>
auto SerializeTo(void * destination, Parameter const & parameter) -> void *
{
    destination = SerializeTo<endianness>(destination, parameter.id);
    destination = SerializeTo<endianness>(destination, parameter.value);
    return destination;
}


template auto SerializeTo<std::endian::big>(void *, Parameter const &) -> void *;
template auto SerializeTo<std::endian::little>(void *, Parameter const &) -> void *;


template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, Parameter * parameter) -> void const *
{
    source = DeserializeFrom<endianness>(source, &parameter->id);
    source = DeserializeFrom<endianness>(source, &parameter->value);
    return source;
}


template auto DeserializeFrom<std::endian::big>(void const *, Parameter *) -> void const *;
template auto DeserializeFrom<std::endian::little>(void const *, Parameter *) -> void const *;
}
