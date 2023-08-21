#include <Sts1CobcSw/Edu/ProgramQueue.hpp>

#include <rodos_no_using_namespace.h>


// TODO: Change namespace here too, or move it back to Sts1CobSw/
namespace sts1cobcsw::edu
{
using sts1cobcsw::serial::Byte;
using sts1cobcsw::serial::DeserializeFrom;
using sts1cobcsw::serial::SerializeTo;

etl::vector<QueueEntry, programQueueSize> programQueue{};
std::uint16_t queueIndex = 0U;


auto DeserializeFrom(void const * source, QueueEntry * data) -> void const *
{
    source = DeserializeFrom(source, &(data->programId));
    source = DeserializeFrom(source, &(data->queueId));
    source = DeserializeFrom(source, &(data->startTime));
    source = DeserializeFrom(source, &(data->timeout));
    return source;
}

auto SerializeTo(void * destination, sts1cobcsw::edu::QueueEntry const & data) -> void *
{
    destination = SerializeTo(destination, data.programId);
    destination = SerializeTo(destination, data.queueId);
    destination = SerializeTo(destination, data.startTime);
    destination = SerializeTo(destination, data.timeout);
    return destination;
}
}