#pragma once


#include <Sts1CobcSw/Periphery/FramLayout.hpp>
#include <Sts1CobcSw/ProgramId/ProgramId.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <etl/vector.h>

#include <bit>
#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
namespace edu
{
// TODO: Rename to ProgramQueueEntry
struct QueueEntry
{
    ProgramId programId = ProgramId(0);
    std::int32_t startTime = 0;
    std::int16_t timeout = 0;
};
}


template<>
inline constexpr std::size_t serialSize<edu::QueueEntry> =
    totalSerialSize<decltype(edu::QueueEntry::programId),
                    decltype(edu::QueueEntry::startTime),
                    decltype(edu::QueueEntry::timeout)>;


namespace edu
{
inline constexpr auto programQueueSize = 20;
static_assert(programQueueSize * totalSerialSize<QueueEntry> <= fram::EduProgramQueue::size,
              "Size of EDU program queue exceeds size of FRAM section");

extern std::uint16_t queueIndex;
extern etl::vector<QueueEntry, programQueueSize> programQueue;


template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, QueueEntry * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, QueueEntry const & data) -> void *;
}
}
