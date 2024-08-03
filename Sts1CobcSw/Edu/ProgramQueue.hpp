#pragma once


#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/ProgramId/ProgramId.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <strong_type/ordered_with.hpp>

#include <etl/vector.h>

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
// TODO: Compute programQueueSize from framSections.template Get<"eduProgramQueue">().size instead
inline constexpr auto programQueueSize = 20;
static_assert(programQueueSize * totalSerialSize<ProgramQueueEntry>
                  <= framSections.template Get<"eduProgramQueue">().size,
              "Size of EDU program queue exceeds size of FRAM section");

extern std::uint16_t queueIndex;
extern etl::vector<ProgramQueueEntry, programQueueSize> programQueue;


template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, ProgramQueueEntry * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, ProgramQueueEntry const & data) -> void *;
}
}
