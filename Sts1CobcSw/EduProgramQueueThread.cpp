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
using RODOS::NOW;
using RODOS::SECONDS;
using RODOS::MILLISECONDS;
// Define our Edu Program Queue
etl::vector<QueueEntry, eduProgramQueueSize> eduProgramQueue;

RODOS::RingBuffer<StatusHistoryEntry, statusHistorySize> statusHistory;

auto AddQueueEntry(const QueueEntry & eduEntry)
{
    eduProgramQueue.push_back(eduEntry);
}

auto GetProgId(const etl::vector<QueueEntry, eduProgramQueueSize> & queue, uint32_t index)
{
    return std::get<0>(queue.at(index));
}

auto GetQueueId(const etl::vector<QueueEntry, eduProgramQueueSize> & queue, uint32_t index)
{
    return std::get<1>(queue.at(index));
}

auto GetTimeout(const etl::vector<QueueEntry, eduProgramQueueSize> & queue, uint32_t index)
{
    return std::get<3>(queue.at(index));
}

auto eduUartInterface = periphery::EduUartInterface();

auto currentQueueId = 0U;

class EduQueueThread : public RODOS::StaticThread<>
{
    void init() override
    {
        int16_t progId = 1;
        int16_t queueId = 3;
        int64_t startTime = (4 + 4 + 2) * SECONDS;
        int64_t maxRunTime = 4 * SECONDS;

        AddQueueEntry(std::tie(progId, queueId, startTime, maxRunTime));
    }

    void run() override
    {
        for(;;)
        {
            RODOS::PRINTF("Hello from EduQueueThread\n");

            constexpr auto eduCommunicationDelay = 2 * SECONDS;
            auto const startTime = NOW() + 3 * SECONDS;
            auto const delayTime = std::max(startTime - NOW(), 0 * MILLISECONDS);

            // Suspend until delay time - 2 seconds
            AT(NOW() + delayTime - eduCommunicationDelay);

            // TODO : Send UTC estimation date to edu
            // eduUartInterface.updateTime();

            // Start Process
            auto const programId = GetProgId(eduProgramQueue, currentQueueId);
            auto const queueId = GetQueueId(eduProgramQueue, currentQueueId);
            auto const timeout = GetTimeout(eduProgramQueue, currentQueueId);

            auto eduAnswer = eduUartInterface.ExecuteProgram(programId, queueId, timeout);

            // Suspend Self for execution time
            auto const exetime = NOW() + timeout + 2 * SECONDS;
            AT(NOW() + exetime);

            // Create status and history entry and set status
            etl::string<statusMaxSize> status = "1x";
            if(eduAnswer == periphery::EduErrorCode::success)
            {
                status = "1";
            }
            statusHistory.put(std::make_tuple(programId, queueId, status));

            // Set current Queue ID to next
            currentQueueId++;
            if(currentQueueId >= eduProgramQueue.size())
            {
                // Suspend thread forever
                AT(RODOS::END_OF_TIME);
            }
        }
    }
};

auto const eduQueueThread = EduQueueThread();
}
