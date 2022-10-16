#include <Sts1CobcSw/EduProgramQueueThread.hpp>

#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/Enums.hpp>

#include <ringbuffer.h>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <algorithm>

namespace sts1cobcsw
{
using RODOS::AT;
using RODOS::MILLISECONDS;
using RODOS::NOW;
using RODOS::SECONDS;

// Define our Edu Program Queue
etl::vector<QueueEntry, eduProgramQueueSize> eduProgramQueue;

RODOS::RingBuffer<StatusHistoryEntry, statusHistorySize> statusHistory;

auto queueIndex = 0U;
auto endOfQueueIsReached = false;

void AddQueueEntry(const QueueEntry & eduEntry)
{
    eduProgramQueue.push_back(eduEntry);
}

//! @brief Set QueueId back to 0 and endOfQueueReached back to false
void ResetQueueIndex()
{
    // The queue ID is not the index,
    queueIndex = 0U;
}

auto ProgramId(etl::vector<QueueEntry, eduProgramQueueSize> const & queue, uint32_t index)
{
    return queue[index].programId;
}

auto QueueId(etl::vector<QueueEntry, eduProgramQueueSize> const & queue, uint32_t index)
{
    return queue[index].queueId;
}

auto StartTime(etl::vector<QueueEntry, eduProgramQueueSize> const & queue, uint32_t index)
{
    return queue[index].startTime;
}

auto Timeout(etl::vector<QueueEntry, eduProgramQueueSize> const & queue, uint32_t index)
{
    return queue[index].timeout;
}

auto eduUartInterface = periphery::EduUartInterface();

constexpr auto eduCommunicationDelay = 2 * SECONDS;


class EduQueueThread : public RODOS::StaticThread<>
{
    void run() override
    {
        while(true)
        {
            RODOS::PRINTF("Hello from EduQueueThread\n");

            if(eduProgramQueue.empty())
            {
                RODOS::PRINTF("Edu Program Queue is empty, thread set to sleep until end of time");
                AT(RODOS::END_OF_TIME);
            }
            else if(queueIndex >= eduProgramQueue.size())
            {
                RODOS::PRINTF("End of queue is reached, thread set to sleep until end of time");
                AT(RODOS::END_OF_TIME);
            }
        }


        // TODO: This is for testing purpose, next step is fetching the real time from the build
        // queue command
        // max(startTime - toUnix(getUTC()))
        auto const startTime = NOW() + 3 * SECONDS;
        auto const delayTime = std::max(startTime - NOW(), 0 * MILLISECONDS);

        // Suspend until delay time - 2 seconds
        AT(NOW() + delayTime - eduCommunicationDelay);

        // TODO: Send UTC estimation date to edu (Unix)
        // auto sysUTC = RODOS::sysTime.getUTC();
        // sysUTC = sysUtC / RODOS::NANOSECONDS
        // sysUTC = sysUtc + rodosOffset
        // eduUartInterface.updateTime(sysUtc);

        // Start Process
        auto const programId = ProgramId(eduProgramQueue, queueIndex);
        auto const queueId = QueueId(eduProgramQueue, queueIndex);
        auto const timeout = Timeout(eduProgramQueue, queueIndex);

        auto eduAnswer = eduUartInterface.ExecuteProgram(programId, queueId, timeout);

        // Suspend Self for execution time
        auto const executionTime = timeout + 2 * SECONDS;
        AT(NOW() + executionTime);

        // Create Status&History entry
        if(eduAnswer == periphery::EduErrorCode::success)
        {
            uint8_t status = 1;
            auto statusHistoryEntry =
                StatusHistoryEntry{.programId = programId, .queueId = queueId, .status = status};
            statusHistory.put(statusHistoryEntry);
        }

        // Set current Queue ID to next
        queueIndex++;
    }
};

auto eduQueueThread = EduQueueThread();

void TimeEvent::handle()
{
    RODOS::PRINTF("Time Event at %3.9f\n", RODOS::SECONDS_NOW());
    eduQueueThread.resume();
    RODOS::PRINTF("EduQueueThread resumed from me\n");
}
}
