#pragma once


#include <Sts1CobcSw/Edu/Structs.hpp>


namespace sts1cobcsw::edu
{
using sts1cobcsw::DeserializeFrom;
using sts1cobcsw::SerializeTo;


template<std::endian endianness>
auto DeserializeFrom(void const * source, HeaderData * data) -> void const *
{
    source = DeserializeFrom<endianness>(source, &(data->command));
    source = DeserializeFrom<endianness>(source, &(data->length));
    return source;
}


template<std::endian endianness>
auto DeserializeFrom(void const * source, ProgramFinishedStatus * data) -> void const *
{
    source = DeserializeFrom<endianness>(source, &(data->programId));
    source = DeserializeFrom<endianness>(source, &(data->timestamp));
    source = DeserializeFrom<endianness>(source, &(data->exitCode));
    return source;
}


template<std::endian endianness>
auto DeserializeFrom(void const * source, ResultsReadyStatus * data) -> void const *
{
    source = DeserializeFrom<endianness>(source, &(data->programId));
    source = DeserializeFrom<endianness>(source, &(data->timestamp));
    return source;
}


template<std::endian endianness>
auto SerializeTo(void * destination, StoreArchiveData const & data) -> void *
{
    destination = SerializeTo<endianness>(destination, StoreArchiveData::id);
    destination = SerializeTo<endianness>(destination, data.programId);
    return destination;
}


template<std::endian endianness>
auto SerializeTo(void * destination, ExecuteProgramData const & data) -> void *
{
    destination = SerializeTo<endianness>(destination, ExecuteProgramData::id);
    destination = SerializeTo<endianness>(destination, data.programId);
    destination = SerializeTo<endianness>(destination, data.timestamp);
    destination = SerializeTo<endianness>(destination, data.timeout);
    return destination;
}


template<std::endian endianness>
auto SerializeTo(void * destination, UpdateTimeData const & data) -> void *
{
    destination = SerializeTo<endianness>(destination, UpdateTimeData::id);
    destination = SerializeTo<endianness>(destination, data.timestamp);
    return destination;
}
}
