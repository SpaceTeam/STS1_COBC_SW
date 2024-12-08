#include <Sts1CobcSw/Edu/ProgramQueue.hpp>


// TODO: Change namespace here too, or move it back to Sts1CobSw/
namespace sts1cobcsw::edu
{
using sts1cobcsw::DeserializeFrom;
using sts1cobcsw::SerializeTo;


sts1cobcsw::ProgramQueue<ProgramQueueEntry,
                         framSections.Get<"eduProgramStatusHistory">(),
                         nCachedProgramEntries>
    programQueue;


std::uint16_t queueIndex = 0;


template<std::endian endianness>
auto DeserializeFrom(void const * source, ProgramQueueEntry * data) -> void const *
{
    source = DeserializeFrom<endianness>(source, &(data->programId));
    source = DeserializeFrom<endianness>(source, &(data->startTime));
    source = DeserializeFrom<endianness>(source, &(data->timeout));
    return source;
}

// Explicit template specializations to keep everything in .cpp file
template auto DeserializeFrom<std::endian::big>(void const *, ProgramQueueEntry *) -> void const *;
template auto DeserializeFrom<std::endian::little>(void const *, ProgramQueueEntry *)
    -> void const *;


template<std::endian endianness>
auto SerializeTo(void * destination, ProgramQueueEntry const & data) -> void *
{
    destination = SerializeTo<endianness>(destination, data.programId);
    destination = SerializeTo<endianness>(destination, data.startTime);
    destination = SerializeTo<endianness>(destination, data.timeout);
    return destination;
}

// Explicit template specializations to keep everything in .cpp file
template auto SerializeTo<std::endian::big>(void *, ProgramQueueEntry const &) -> void *;
template auto SerializeTo<std::endian::little>(void *, ProgramQueueEntry const &) -> void *;
}
