#pragma once


// clang-format off
#include <cstdint>
// ringbuffer.h does not include <cstdint> even though it requires it
#include <rodos/support/support-libs/ringbuffer.h>
// clang-format on

#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <type_safe/types.hpp>

#include <etl/string.h>
#include <etl/vector.h>


namespace sts1cobcsw
{
namespace edu
{
namespace ts = type_safe;
using ts::operator""_u8;
using ts::operator""_i16;
using ts::operator""_u16;
using ts::operator""_i32;


struct QueueEntry
{
    ts::uint16_t programId = 0_u16;
    ts::uint16_t queueId = 0_u16;
    ts::int32_t startTime = 0_i32;
    ts::int16_t timeout = 0_i16;
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

extern uint16_t queueIndex;
extern etl::vector<QueueEntry, programQueueSize> programQueue;


auto DeserializeFrom(void const * source, QueueEntry * data) -> void const *;
auto SerializeTo(void * destination, QueueEntry const & data) -> void *;
}
}
