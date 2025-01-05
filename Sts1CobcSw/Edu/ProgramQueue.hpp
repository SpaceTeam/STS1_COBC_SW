#pragma once


#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/FramVector.hpp>
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
inline constexpr auto nCachedProgramQueueEntries = 10;
extern FramVector<ProgramQueueEntry,
                  framSections.template Get<"eduProgramQueue">(),
                  nCachedProgramQueueEntries>
    programQueue;


template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, ProgramQueueEntry * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, ProgramQueueEntry const & data) -> void *;
}
}
