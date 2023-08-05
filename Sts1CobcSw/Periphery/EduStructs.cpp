#include <Sts1CobcSw/Periphery/EduStructs.hpp>


namespace sts1cobcsw::periphery
{
using sts1cobcsw::serial::Byte;
using sts1cobcsw::serial::DeserializeFrom;
using sts1cobcsw::serial::SerializeTo;


auto DeserializeFrom(void const * source, HeaderData * data) -> void const *
{
    source = DeserializeFrom(source, &(data->command));
    source = DeserializeFrom(source, &(data->length));
    return source;
}


auto DeserializeFrom(void const * source, ProgramFinishedStatus * data) -> void const *
{
    source = DeserializeFrom(source, &(data->programId));
    source = DeserializeFrom(source, &(data->queueId));
    source = DeserializeFrom(source, &(data->exitCode));
    return source;
}


auto DeserializeFrom(void const * source, ResultsReadyStatus * data) -> void const *
{
    source = DeserializeFrom(source, &(data->programId));
    source = DeserializeFrom(source, &(data->queueId));
    return source;
}


auto SerializeTo(void * destination, StoreArchiveData const & data) -> void *
{
    destination = SerializeTo(destination, StoreArchiveData::id);
    destination = SerializeTo(destination, data.programId);
    return destination;
}


auto SerializeTo(void * destination, ExecuteProgramData const & data) -> void *
{
    destination = SerializeTo(destination, ExecuteProgramData::id);
    destination = SerializeTo(destination, data.programId);
    destination = SerializeTo(destination, data.queueId);
    destination = SerializeTo(destination, data.timeout);
    return destination;
}


auto SerializeTo(void * destination, ReturnResultData const & data) -> void *
{
    destination = SerializeTo(destination, ReturnResultData::id);
    destination = SerializeTo(destination, data.programId);
    destination = SerializeTo(destination, data.queueId);
    return destination;
}


auto SerializeTo(void * destination, UpdateTimeData const & data) -> void *
{
    destination = SerializeTo(destination, UpdateTimeData::id);
    destination = SerializeTo(destination, data.timestamp);
    return destination;
}
}
