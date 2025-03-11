#pragma once


#include <Sts1CobcSw/Periphery/Eps.hpp>


namespace sts1cobcsw::eps
{
template<std::endian endianness>
auto DeserializeFrom(void const * source, AdcData * data) -> void const *
{
    using sts1cobcsw::DeserializeFrom;

    source = DeserializeFrom<endianness>(source, &data->adc4);
    source = DeserializeFrom<endianness>(source, &data->adc5);
    return DeserializeFrom<endianness>(source, &data->adc6);
}


template<std::endian endianness>
auto SerializeTo(void * destination, AdcData const & data) -> void *
{
    using sts1cobcsw::SerializeTo;

    destination = SerializeTo<endianness>(destination, data.adc4);
    destination = SerializeTo<endianness>(destination, data.adc5);
    return SerializeTo<endianness>(destination, data.adc6);
}
}
