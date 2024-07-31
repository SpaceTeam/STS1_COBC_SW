#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>

#include <strong_type/equality.hpp>


namespace sts1cobcsw::edu
{
RODOS::RingBuffer<ProgramStatusHistoryEntry, programStatusHistorySize> programStatusHistory;


auto UpdateProgramStatusHistory(ProgramId programId,
                                std::int32_t startTime,
                                ProgramStatus newStatus) -> void
{
    // TODO: Check that there is only one entry matching program/queue ID, or should it be the case
    // by construction ?
    for(std::uint32_t i = 0; i < programStatusHistory.occupiedCnt; ++i)
    {
        if(programStatusHistory.vals[i].startTime == startTime
           and programStatusHistory.vals[i].programId == programId)
        {
            programStatusHistory.vals[i].status = newStatus;
        }
    }
}


using sts1cobcsw::DeserializeFrom;
using sts1cobcsw::SerializeTo;


template<std::endian endianness>
auto DeserializeFrom(void const * source, edu::ProgramStatusHistoryEntry * data) -> void const *
{
    source = DeserializeFrom<endianness>(source, &(data->programId));
    source = DeserializeFrom<endianness>(source, &(data->startTime));
    source = DeserializeFrom<endianness>(source, &(data->status));
    return source;
}


template auto DeserializeFrom<std::endian::big>(void const *, edu::ProgramStatusHistoryEntry *)
    -> void const *;
template auto DeserializeFrom<std::endian::little>(void const *, edu::ProgramStatusHistoryEntry *)
    -> void const *;


template<std::endian endianness>
auto SerializeTo(void * destination, edu::ProgramStatusHistoryEntry const & data) -> void *
{
    destination = SerializeTo<endianness>(destination, data.programId);
    destination = SerializeTo<endianness>(destination, data.startTime);
    destination = SerializeTo<endianness>(destination, data.status);
    return destination;
}


template auto SerializeTo<std::endian::big>(void *, edu::ProgramStatusHistoryEntry const &)
    -> void *;
template auto SerializeTo<std::endian::little>(void *, edu::ProgramStatusHistoryEntry const &)
    -> void *;
}