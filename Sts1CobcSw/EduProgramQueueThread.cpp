#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/Periphery/Enums.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <ringbuffer.h>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <algorithm>
#include <cinttypes>
#include <iostream>


namespace sts1cobcsw
{
using sts1cobcsw::serial::Byte;

using RODOS::AT;
using RODOS::MILLISECONDS;
using RODOS::NOW;
using RODOS::SECONDS;


auto queueIndex = 0U;
etl::vector<QueueEntry, eduProgramQueueSize> eduProgramQueue =
    etl::vector<QueueEntry, eduProgramQueueSize>();
RODOS::RingBuffer<StatusHistoryEntry, statusHistorySize> statusHistory =
    RODOS::RingBuffer<StatusHistoryEntry, statusHistorySize>();


void AddQueueEntry(const QueueEntry & eduEntry)
{
    eduProgramQueue.push_back(eduEntry);
}


void ResetQueueIndex()
{
    queueIndex = 0U;
    RODOS::PRINTF("Current size of edu program queue is : %d\n", eduProgramQueue.size());
}


void EmptyEduProgramQueue()
{
    eduProgramQueue.clear();
}


auto edu = periphery::Edu();
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

        AddQueueEntry(queueEntry1);
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


            auto startTime = eduProgramQueue[queueIndex].startTime - util::rodosUnixOffsetSeconds;
            auto currentUtcTime = RODOS::sysTime.getUTC() / RODOS::SECONDS;
            auto const startDelay = std::max((startTime - currentUtcTime) * SECONDS, 0 * SECONDS);

            RODOS::PRINTF("Next program will start in  : %" PRIi64 " nanoseconds\n", startDelay);

            // Suspend until delay time - 2 seconds
            RODOS::PRINTF("Suspending for %" PRIi64 " nanoseconds\n",
                          startDelay - eduCommunicationDelay);
            AT(NOW() + startDelay - eduCommunicationDelay);

            RODOS::PRINTF("Resuming here after first start delay");

            // Send UTC to EDU


            // delay again
            // What is expected exactly, this :
            AT(NOW() + startDelay - eduCommunicationDelay);
            // This will wait for 20 years if not recomputed

            // or this ( make more sense) :

            // First we recompute startdelay
            //rawStartTime = eduProgramQueue[queueIndex].startTime - util::rodosUnixOffsetSeconds;
            //sysUtcSeconds = RODOS::sysTime.getUTC() / RODOS::SECONDS;
            //auto const startDelay2 =
            //    std::max((rawStartTime - sysUtcSeconds) * RODOS::SECONDS, 0 * RODOS::SECONDS);
            // Then we wait
            //AT(NOW() + startDelay2);


            // Example : we enter the while loop with a program starting in 10 s
            // Sleep for 8 seconds
            // resume
            // Communicate with edu for sending time for a communicationdelay of delta seconds
            // recompute startdelay (wich is equal to max(10-8-delta, 0))
            // sleep for startdelay

            auto queueId = eduProgramQueue[queueIndex].queueId;
            auto programId = eduProgramQueue[queueIndex].programId;
            auto timeout = eduProgramQueue[queueIndex].timeout;

            auto executeProgramData = periphery::ExecuteProgramData{
                .programId = programId, .queueId = queueId, .timeout = timeout};
            // Start Process
            auto eduAnswer = edu.ExecuteProgram(executeProgramData);

            // Suspend Self for execution time
            auto const executionTime = timeout + eduCommunicationDelay;
            AT(NOW() + executionTime);

            // TODO: Switch statement
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
