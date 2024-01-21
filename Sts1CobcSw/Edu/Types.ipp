#pragma once


#include <Sts1CobcSw/Edu/Types.hpp>


namespace sts1cobcsw::edu
{
using sts1cobcsw::DeserializeFrom;
using sts1cobcsw::SerializeTo;


template<std::endian endianness>
auto SerializeTo(void * destination, StoreProgramData const & data) -> void *
{
    destination = SerializeTo<endianness>(destination, StoreProgramData::id);
    destination = SerializeTo<endianness>(destination, data.programId);
    return destination;
}


template<std::endian endianness>
auto SerializeTo(void * destination, ExecuteProgramData const & data) -> void *
{
    destination = SerializeTo<endianness>(destination, ExecuteProgramData::id);
    destination = SerializeTo<endianness>(destination, data.programId);
    destination = SerializeTo<endianness>(destination, data.startTime);
    destination = SerializeTo<endianness>(destination, data.timeout);
    return destination;
}


template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, [[maybe_unused]] StopProgramData const & data)
    -> void *
{
    destination = SerializeTo<endianness>(destination, StopProgramData::id);
    return destination;
}


template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, [[maybe_unused]] GetStatusData const & data)
    -> void *
{
    destination = SerializeTo<endianness>(destination, GetStatusData::id);
    return destination;
}


template<std::endian endianness>
auto SerializeTo(void * destination, ReturnResultData const & data) -> void *
{
    destination = SerializeTo<endianness>(destination, ReturnResultData::id);
    destination = SerializeTo<endianness>(destination, data.programId);
    destination = SerializeTo<endianness>(destination, data.startTime);
    return destination;
}


template<std::endian endianness>
auto SerializeTo(void * destination, UpdateTimeData const & data) -> void *
{
    destination = SerializeTo<endianness>(destination, UpdateTimeData::id);
    destination = SerializeTo<endianness>(destination, data.currentTime);
    return destination;
}


template<std::endian endianness>
auto DeserializeFrom(void const * source, ProgramFinishedStatus * data) -> void const *
{
    source = DeserializeFrom<endianness>(source, &(data->programId));
    source = DeserializeFrom<endianness>(source, &(data->startTime));
    source = DeserializeFrom<endianness>(source, &(data->exitCode));
    return source;
}


template<std::endian endianness>
auto DeserializeFrom(void const * source, ResultsReadyStatus * data) -> void const *
{
    source = DeserializeFrom<endianness>(source, &(data->programId));
    source = DeserializeFrom<endianness>(source, &(data->startTime));
    return source;
}
}
