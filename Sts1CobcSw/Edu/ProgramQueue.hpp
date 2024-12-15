#pragma once


#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/ProgramQueue.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>
#include <Sts1CobcSw/ProgramId/ProgramId.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <strong_type/type.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
namespace edu
{
struct ProgramQueueEntry
{
    ProgramId programId = ProgramId(0);
    RealTime startTime = RealTime(0);
    std::int16_t timeout = 0;
};
}


template<>
inline constexpr std::size_t serialSize<edu::ProgramQueueEntry> =
    totalSerialSize<decltype(edu::ProgramQueueEntry::programId),
                    decltype(edu::ProgramQueueEntry::startTime),
                    decltype(edu::ProgramQueueEntry::timeout)>;


namespace edu
{
inline constexpr auto nProgramQueueEntries =
    value_of(framSections.template Get<"eduProgramQueue">().size)
    / totalSerialSize<ProgramQueueEntry>;


extern std::uint16_t queueIndex;

inline constexpr auto nCachedProgramEntries = 10;
extern ProgramQueue<ProgramQueueEntry,
                    framSections.template Get<"eduProgramStatusHistory">(),
                    nCachedProgramEntries>
    programQueue;


template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, ProgramQueueEntry * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, ProgramQueueEntry const & data) -> void *;
}
}
