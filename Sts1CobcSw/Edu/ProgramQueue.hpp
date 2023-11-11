#pragma once


// clang-format off
#include <cstdint>
// ringbuffer.h does not include <cstdint> even though it requires it
#include <rodos/support/support-libs/ringbuffer.h>
// clang-format on

#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <etl/string.h>
#include <etl/vector.h>

#include <cstddef>


namespace sts1cobcsw
{
namespace edu
{
struct QueueEntry
{
    std::uint16_t programId = 0;
    std::uint16_t queueId = 0;
    std::int32_t startTime = 0;
    std::int16_t timeout = 0;
};
}


template<>
inline constexpr std::size_t serialSize<edu::QueueEntry> =
    totalSerialSize<decltype(edu::QueueEntry::programId),
                    decltype(edu::QueueEntry::queueId),
                    decltype(edu::QueueEntry::startTime),
                    decltype(edu::QueueEntry::timeout)>;


namespace edu
{
inline constexpr auto programQueueSize = 20;

extern std::uint16_t queueIndex;
extern etl::vector<QueueEntry, programQueueSize> programQueue;


auto DeserializeFrom(void const * source, QueueEntry * data) -> void const *;
auto SerializeTo(void * destination, QueueEntry const & data) -> void *;
}
}
