#include <Sts1CobcSw/Periphery/EduStructs.hpp>


namespace sts1cobcsw::periphery
{
using sts1cobcsw::serial::Byte;
using sts1cobcsw::serial::DeserializeFrom;
using sts1cobcsw::serial::SerializeTo;


auto DeserializeFrom(Byte * source, HeaderData * data) -> Byte *
{
    source = DeserializeFrom(source, &(data->command));
    source = DeserializeFrom(source, &(data->length));
    return source;
}


auto DeserializeFrom(Byte * source, ProgramFinishedStatus * data) -> Byte *
{
    source = DeserializeFrom(source, &(data->programId));
    source = DeserializeFrom(source, &(data->queueId));
    source = DeserializeFrom(source, &(data->exitCode));
    return source;
}


auto DeserializeFrom(Byte * source, ResultsReadyStatus * data) -> Byte *
{
    source = DeserializeFrom(source, &(data->programId));
    source = DeserializeFrom(source, &(data->queueId));
    return source;
}


auto SerializeTo(Byte * destination, StoreArchiveData const & data) -> Byte *
{
    destination = SerializeTo(destination, StoreArchiveData::id);
    destination = SerializeTo(destination, data.programId);
    return destination;
}


auto SerializeTo(Byte * destination, ExecuteProgramData const & data) -> Byte *
{
    destination = SerializeTo(destination, ExecuteProgramData::id);
    destination = SerializeTo(destination, data.programId);
    destination = SerializeTo(destination, data.queueId);
    destination = SerializeTo(destination, data.timeout);
    return destination;
}


auto SerializeTo(Byte * destination, UpdateTimeData const & data) -> Byte *
{
    destination = SerializeTo(destination, UpdateTimeData::id);
    destination = SerializeTo(destination, data.timestamp);
    return destination;
}
}
