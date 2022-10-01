#include <Sts1CobcSw/EduProgramQueueThread.hpp>

#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/Enums.hpp>

#include <ringbuffer.h>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <algorithm>

namespace sts1cobcsw
{
using RODOS::SECONDS;
// Define our Edu Program Queue
etl::vector<QueueEntry, eduProgramQueueSize> eduQueue;

RODOS::RingBuffer<StatusHistoryEntry, statusHistorySize> statusHistory;

auto AddQueueEntry(const QueueEntry & eduEntry)
{
    eduQueue.push_back(eduEntry);
}

auto eduUartInterface = periphery::EduUartInterface();

auto currentQueueId = 0U;

class EduQueueThread : public RODOS::StaticThread<>
{
    void init() override
    {
        int16_t progId = 1;
        int16_t queueId = 3;
        int64_t startTime = (4 + 4 + 2) * RODOS::SECONDS;
        int64_t maxRunTime = 4 * RODOS::SECONDS;

        AddQueueEntry(std::tie(progId, queueId, startTime, maxRunTime));
    }

    void run() override
    {
        RODOS::PRINTF("Hello from EduQueueThread\n");

        auto const startTime = 10 * RODOS::SECONDS;

        auto delayTime = std::max(startTime - RODOS::NOW(), 0 * RODOS::MILLISECONDS);
        RODOS::AT(RODOS::NOW() + delayTime - 2 * RODOS::SECONDS);

        // TODO : Send UTC estimation date to edu
        // eduUartInterface.updateTime();

        RODOS::AT(RODOS::NOW() + delayTime);

        // Start Process
        auto const programId = std::get<0>(eduQueue.at(currentQueueId));
        auto const queueId = std::get<1>(eduQueue.at(currentQueueId));
        auto const timeout = std::get<3>(eduQueue.at(currentQueueId));

        // Execute Program
        auto eduAnswer = eduUartInterface.ExecuteProgram(programId, queueId, timeout);

        etl::string<statusMaxSize> status = "1x";
        if(eduAnswer == periphery::EduErrorCode::success)
        {
            status = "1";
        }
        // TODO Create a status and history entry if not already existing
        statusHistory.put(std::make_tuple(programId, queueId, status));

        // Suspend Self until exetime = now() + timeout + 2 sec
        RODOS::AT(RODOS::NOW() + (timeout + 2) * SECONDS);

        // Set current Queue ID to next
        currentQueueId++;
    }
};

auto const eduQueueThread = EduQueueThread();
}
