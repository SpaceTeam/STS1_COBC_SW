#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/RingArray.hpp>

#include <strong_type/equality.hpp>


namespace sts1cobcsw::edu
{
sts1cobcsw::RingArray<ProgramStatusHistoryEntry,
                      framSections.Get<"eduProgramStatusHistory">(),
                      nCachedProgramStatusHistoryEntries>
    programStatusHistory;


auto UpdateProgramStatusHistory(ProgramId programId, RealTime startTime, ProgramStatus newStatus)
    -> void
{
    programStatusHistory.FindAndReplace(
        [&](ProgramStatusHistoryEntry const & entry) -> bool
        { return entry.startTime == startTime && entry.programId == programId; },
        ProgramStatusHistoryEntry{programId, startTime, newStatus});
}


using sts1cobcsw::DeserializeFrom;
using sts1cobcsw::SerializeTo;


template<std::endian endianness>
auto DeserializeFrom(void const * source, ProgramStatusHistoryEntry * data) -> void const *
{
    source = DeserializeFrom<endianness>(source, &(data->programId));
    source = DeserializeFrom<endianness>(source, &(data->startTime));
    source = DeserializeFrom<endianness>(source, &(data->status));
    return source;
}


template<std::endian endianness>
auto SerializeTo(void * destination, ProgramStatusHistoryEntry const & data) -> void *
{
    destination = SerializeTo<endianness>(destination, data.programId);
    destination = SerializeTo<endianness>(destination, data.startTime);
    destination = SerializeTo<endianness>(destination, data.status);
    return destination;
}


template auto DeserializeFrom<std::endian::big>(void const *, ProgramStatusHistoryEntry *)
    -> void const *;
template auto DeserializeFrom<std::endian::little>(void const *, ProgramStatusHistoryEntry *)
    -> void const *;
template auto SerializeTo<std::endian::big>(void *, ProgramStatusHistoryEntry const &) -> void *;
template auto SerializeTo<std::endian::little>(void *, ProgramStatusHistoryEntry const &) -> void *;
}
