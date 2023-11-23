#include <Sts1CobcSw/Edu/ProgramQueue.hpp>

#include <rodos_no_using_namespace.h>


// TODO: Change namespace here too, or move it back to Sts1CobSw/
namespace sts1cobcsw::edu
{
using sts1cobcsw::DeserializeFrom;
using sts1cobcsw::SerializeTo;


etl::vector<QueueEntry, programQueueSize> programQueue{};
std::uint16_t queueIndex = 0U;


template<std::endian endianness>
auto DeserializeFrom(void const * source, QueueEntry * data) -> void const *
{
    source = DeserializeFrom<endianness>(source, &(data->programId));
    source = DeserializeFrom<endianness>(source, &(data->queueId));
    source = DeserializeFrom<endianness>(source, &(data->startTime));
    source = DeserializeFrom<endianness>(source, &(data->timeout));
    return source;
}

// Explicit template specializations to keep everything in .cpp file
template auto DeserializeFrom<std::endian::big>(void const *, QueueEntry *) -> void const *;
template auto DeserializeFrom<std::endian::little>(void const *, QueueEntry *) -> void const *;


template<std::endian endianness>
auto SerializeTo(void * destination, QueueEntry const & data) -> void *
{
    destination = SerializeTo<endianness>(destination, data.programId);
    destination = SerializeTo<endianness>(destination, data.queueId);
    destination = SerializeTo<endianness>(destination, data.startTime);
    destination = SerializeTo<endianness>(destination, data.timeout);
    return destination;
}

// Explicit template specializations to keep everything in .cpp file
template auto SerializeTo<std::endian::big>(void *, QueueEntry const &) -> void *;
template auto SerializeTo<std::endian::little>(void *, QueueEntry const &) -> void *;
}
