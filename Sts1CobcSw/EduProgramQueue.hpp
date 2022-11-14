#pragma once

// clang-format off
#include <cstdint>
// TODO: Change install rules of rodos such that this must be included as
// rodos/support-libs/ringbuffer.h or something.
//
// ringbuffer.h does not include <cstdint> even though it requires it
#include <ringbuffer.h>
// clang-format on


#include <etl/string.h>
#include <etl/vector.h>


namespace sts1cobcsw
{
// TODO: Use type_safe::, This way you cannot construct uninitialized QueueEntries
struct EduQueueEntry
{
    uint16_t programId;
    uint16_t queueId;
    int32_t startTime;
    int16_t timeout;
};


// TODO: Again, type_safe::
struct StatusHistoryEntry
{
    uint16_t programId;
    uint16_t queueId;
    uint8_t status;
};


inline constexpr auto eduProgramQueueCapacity = 20;
extern etl::vector<EduQueueEntry, eduProgramQueueCapacity> eduProgramQueue;
extern uint16_t queueIndex;

// TODO: Think about the name. Maybe something like program/queueStatusAndHistory is better?
inline constexpr auto statusHistoryCapacity = 20;
// TODO: Maybe move that to its own file? Together with the definition of StatusHistoryEntry of
// course.
extern RODOS::RingBuffer<StatusHistoryEntry, statusHistoryCapacity> statusHistory;
}
