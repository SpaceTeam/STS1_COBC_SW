#include <Sts1CobcSw/EduProgramQueueThread.hpp>

#include <Sts1CobcSw/Periphery/Edu.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <algorithm>

namespace sts1cobcsw
{
etl::vector<QueueEntry, eduProgramQueueSize> eduQueue;

auto AddQueueEntry(const std::tuple<int16_t, int16_t, int64_t, int64_t> & eduEntry)
{
    eduQueue.push_back(eduEntry);
}

auto eduUartInterface = periphery::EduUartInterface();

auto currentQueueId = 0;

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

        RODOS::AT(RODOS::NOW() + delayTime);

        // Start Process
        auto const programID = std::get<0>(eduQueue.at(0));
        auto const queueID = std::get<1>(eduQueue.at(0));
        auto const timeout = std::get<3>(eduQueue.at(0));

        // Execute Program
        eduUartInterface.ExecuteProgram(programID, queueID, timeout);
        // TODO Create a status and history entry if not already existing
        // TODO Set status =1 if RasPi answers with success else 1x

        // Suspend Self until exetime
        RODOS::AT(RODOS::NOW() + timeout * RODOS::SECONDS + 2 * RODOS::SECONDS);


        // Set current Queue ID to next
        currentQueueId++;
    }
};

auto const eduQueueThread = EduQueueThread();
}
