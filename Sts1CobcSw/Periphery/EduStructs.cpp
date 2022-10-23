#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>


namespace sts1cobcsw::periphery
{
using sts1cobcsw::serial::Byte;
using sts1cobcsw::serial::DeserializeFrom;
using sts1cobcsw::serial::SerializeTo;


auto DeserializeFrom(Byte * source, ResultsReadyStatus * data) -> Byte *
{
    source = DeserializeFrom(source, &(data->programId));
    source = DeserializeFrom(source, &(data->queueId));
    return source;
}


auto DeserializeFrom(Byte * source, ProgramFinishedStatus * data) -> Byte *
{
    source = DeserializeFrom(source, &(data->programId));
    source = DeserializeFrom(source, &(data->queueId));
    source = DeserializeFrom(source, &(data->exitCode));
    return source;
}


auto SerializeTo(Byte * destination, ExecuteProgramArguments const & data) -> Byte *
{
    destination = SerializeTo(destination, data.programId);
    destination = SerializeTo(destination, data.queueId);
    destination = SerializeTo(destination, data.timeout);
    return destination;
}
}
