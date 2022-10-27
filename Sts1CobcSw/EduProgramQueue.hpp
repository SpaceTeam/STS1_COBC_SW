#pragma once

// clang-format off
#include <rodos_no_using_namespace.h>
// TODO: Change install rules of rodos such that this must be included as
// rodos/support-libs/ringbuffer.h or something.
//
// ringbuffer.h does not include <cstdint> even though it requires it
#include <ringbuffer.h>
// clang-format on


#include <etl/string.h>
#include <etl/vector.h>

#include <cstdint>


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


inline constexpr auto eduProgramQueueSize = 20;
// TODO: Think about the name. Maybe something like program/queueStatusAndHistory is better?
inline constexpr auto statusHistorySize = 20;

// TODO: Why is this defined in EduProgramQueueThread.cpp and not EduProgramQueue.cpp?
extern uint16_t queueIndex;
extern etl::vector<EduQueueEntry, eduProgramQueueSize> eduProgramQueue;
// TODO: Maybe move that to its own file? Together with the definition of StatusHistoryEntry of
// course.
extern RODOS::RingBuffer<StatusHistoryEntry, statusHistorySize> statusHistory;


void EmptyEduProgramQueue();
void AddQueueEntry(EduQueueEntry const & eduEntry);
void ResetQueueIndex();
}
