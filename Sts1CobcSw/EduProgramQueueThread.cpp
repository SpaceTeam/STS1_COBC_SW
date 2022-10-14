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
    endOfQueueIsReached = false;
}

// const to the right of the type
// TODO: Not using at()
auto ProgramId(const etl::vector<QueueEntry, eduProgramQueueSize> & queue, uint32_t index)
{
    return std::get<0>(queue.at(index));
}

auto QueueId(const etl::vector<QueueEntry, eduProgramQueueSize> & queue, uint32_t index)
{
    return std::get<1>(queue.at(index));
}

auto Timeout(const etl::vector<QueueEntry, eduProgramQueueSize> & queue, uint32_t index)
{
    return std::get<3>(queue.at(index));
}

auto eduUartInterface = periphery::EduUartInterface();


class EduQueueThread : public RODOS::StaticThread<>
{
    void init() override
    {
        int16_t progId = 1;
        int16_t queueId = 3;
        // TODO: constexpr variable
        int64_t startTime = (4 + 4 + 2) * SECONDS;
        int64_t maxRunTime = 4 * SECONDS;

        AddQueueEntry(std::tie(progId, queueId, startTime, maxRunTime));
    }

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
            // TODO:
            // if vector.size() == currentQueueId
            else if(endOfQueueIsReached)
            {
                RODOS::PRINTF("End of queue reached, thread set to sleep until end of time");
                AT(RODOS::END_OF_TIME);
            }

            constexpr auto eduCommunicationDelay = 2 * SECONDS;

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
            // TODO: Name the function after what it returns
            auto const programId = ProgramId(eduProgramQueue, queueIndex);
            auto const queueId = QueueId(eduProgramQueue, queueIndex);
            auto const timeout = Timeout(eduProgramQueue, queueIndex);

            auto eduAnswer = eduUartInterface.ExecuteProgram(programId, queueId, timeout);

            // Suspend Self for execution time
            // TODO: Check that timeout is correctly converted in seconds
            auto const executionTime = timeout + 2 * SECONDS;
            AT(NOW() + executionTime);

            // Create status and history entry and set status
            // Use enums
            // etl::string<statusMaxSize> status = "1x";
            // if(eduAnswer == periphery::EduErrorCode::success)
            //{
            //    status = "1";
            //}
            // statusHistory.put(std::make_tuple(programId, queueId, status));

            // Set current Queue ID to next
            queueIndex++;
            if(queueIndex >= eduProgramQueue.size())
            {
                endOfQueueIsReached = true;
                AT(RODOS::END_OF_TIME);
            }
        }
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
