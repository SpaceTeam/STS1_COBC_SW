#pragma once

// NOLINTNEXTLINE
#include <rodos_no_using_namespace.h>
#include <ringbuffer.h>

#include <etl/string.h>
#include <etl/vector.h>

#include <tuple>


namespace sts1cobcsw {


class TimeEvent : public RODOS::TimeEvent
{
public:
    void handle() override;
};


enum class EduProgramStatus
{
    programRunning = 1,
    programCouldNotBeStarted = 2,
    programExecutionFailed = 3,
    programExecutionFinishedSuccessfully = 4,
    resultFileTransferFinished = 5,
    resultSentToRf = 6,
    ackFromGround = 7,
    resultFileDeleted = 8,
};


// TODO: Type safety
struct QueueEntry
{
    uint16_t programId;
    uint16_t queueId;
    int32_t startTime;
    int16_t timeout;
};


inline constexpr auto eduProgramQueueSize = 20;
extern etl::vector<QueueEntry, eduProgramQueueSize> eduProgramQueue;

struct StatusHistoryEntry
{
    uint16_t programId;
    uint16_t queueId;
    uint8_t status;
};

inline constexpr auto statusHistorySize = 20;
extern RODOS::RingBuffer<StatusHistoryEntry, statusHistorySize> statusHistory;

extern uint16_t queueIndex; 

void EmptyEduProgramQueue();
void AddQueueEntry(const QueueEntry & eduEntry);
void ResetQueueIndex();


}
