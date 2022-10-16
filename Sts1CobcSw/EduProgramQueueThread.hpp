#pragma once

#include <rodos_no_using_namespace.h>

#include <etl/string.h>

#include <tuple>


namespace sts1cobcsw
{
constexpr auto eduProgramQueueSize = 20;
constexpr auto statusHistorySize = 20;

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

struct QueueEntry
{
    uint16_t programId;
    uint16_t queueId;
    uint32_t startTime;
    uint16_t timeout;
};

constexpr auto queueEntrySize = 10;

struct StatusHistoryEntry
{
    uint16_t programId;
    uint16_t queueId;
    uint8_t status;
};


void AddQueueEntry(const QueueEntry & eduEntry);

void ResetQueueIndex();

}
