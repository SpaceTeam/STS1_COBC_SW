#pragma once

// clang-format off
#include <cstdint>
// TODO: Change install rules of rodos such that this must be included as
// rodos/support-libs/ringbuffer.h or something.
//
// ringbuffer.h does not include <cstdint> even though it requires it
#include <ringbuffer.h>
// clang-format on

#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <type_safe/types.hpp>

#include <etl/string.h>
#include <etl/vector.h>


namespace sts1cobcsw
{
namespace ts = type_safe;
using ts::operator""_u8;
using ts::operator""_i16;
using ts::operator""_u16;
using ts::operator""_i32;

using serial::Byte;
using serial::DeserializeFrom;
using serial::SerializeTo;


struct EduQueueEntry
{
    ts::uint16_t programId = 0_u16;
    ts::uint16_t queueId = 0_u16;
    ts::int32_t startTime = 0_i32;
    ts::int16_t timeout = 0_i16;
};

namespace serial
{
template<>
inline constexpr std::size_t serialSize<EduQueueEntry> =
    totalSerialSize<decltype(EduQueueEntry::programId),
                    decltype(EduQueueEntry::queueId),
                    decltype(EduQueueEntry::startTime),
                    decltype(EduQueueEntry::timeout)>;
}


inline constexpr auto eduProgramQueueSize = 20;

extern uint16_t queueIndex;
extern etl::vector<EduQueueEntry, eduProgramQueueSize> eduProgramQueue;


inline auto DeserializeFrom(void const * source, EduQueueEntry * data) -> void const *
{
    source = DeserializeFrom(source, &(data->programId));
    source = DeserializeFrom(source, &(data->queueId));
    source = DeserializeFrom(source, &(data->startTime));
    source = DeserializeFrom(source, &(data->timeout));
    return source;
}


inline auto SerializeTo(void * destination, EduQueueEntry const & data) -> void *
{
    destination = SerializeTo(destination, data.programId);
    destination = SerializeTo(destination, data.queueId);
    destination = SerializeTo(destination, data.startTime);
    destination = SerializeTo(destination, data.timeout);
    return destination;
}
}
