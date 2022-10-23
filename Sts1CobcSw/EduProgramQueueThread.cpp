#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/Enums.hpp>
#include <Sts1CobcSw/Util/Time.hpp>

#include <ringbuffer.h>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <algorithm>
#include <cinttypes>
#include <iostream>


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
    // The queue ID is not the index.
    queueIndex = 0U;
    RODOS::PRINTF("Current size of edu program queue is : %d\n", eduProgramQueue.size());
}

void EmptyEduProgramQueue()
{
    eduProgramQueue.clear();
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
    void init() override
    {
        auto queueEntry1 = QueueEntry{
            .programId = 1, .queueId = 1, .startTime = 1672531215, .timeout = 10};  // NOLINT
        AddQueueEntry(queueEntry1);

        auto queueEntry2 = QueueEntry{
            .programId = 2, .queueId = 1, .startTime = 1672531230, .timeout = 20};  // NOLINT
        AddQueueEntry(queueEntry2);
    }

    void run() override
    {
        while(true)
        {
            RODOS::PRINTF("Hello from EduQueueThread\n");

            if(eduProgramQueue.empty())
            {
                RODOS::PRINTF(
                    "Edu Program Queue is empty, thread set to sleep until end of time\n");
                AT(RODOS::END_OF_TIME);
            }
            else if(queueIndex >= eduProgramQueue.size())
            {
                RODOS::PRINTF("End of queue is reached, thread set to sleep until end of time\n");
                AT(RODOS::END_OF_TIME);
            }


            auto rawStartTime = StartTime(eduProgramQueue, queueIndex);
            rawStartTime = rawStartTime - util::rodosUnixOffsetSeconds;
            auto sysUtcSeconds = RODOS::sysTime.getUTC() / RODOS::SECONDS;
            auto const startTime = rawStartTime - sysUtcSeconds;
            RODOS::PRINTF("Our next program will start in  : %" PRIi64 " seconds\n", startTime);

            auto const delayTime = std::max(startTime * SECONDS, 0 * MILLISECONDS);

            // Suspend until delay time - 2 seconds
            RODOS::PRINTF("Suspending for %" PRIi64 " nanoseconds\n",
                          delayTime - eduCommunicationDelay);
            AT(NOW() + delayTime - eduCommunicationDelay);

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
                RODOS::PRINTF("Edu returned a success error code\n");
                uint8_t status = 1;
                auto statusHistoryEntry = StatusHistoryEntry{
                    .programId = programId, .queueId = queueId, .status = status};
                statusHistory.put(statusHistoryEntry);
            }

            // Set current Queue ID to next
            queueIndex++;
        }
    }
};

auto eduQueueThread = EduQueueThread();

void TimeEvent::handle()
{
    eduQueueThread.resume();
    RODOS::PRINTF("EduQueueThread resumed from me\n");
}
}