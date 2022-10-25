#include <Sts1CobcSw/EduProgramQueue.hpp>
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
namespace ts = type_safe;
using sts1cobcsw::serial::Byte;

using RODOS::AT;
using RODOS::MILLISECONDS;
using RODOS::NOW;
using RODOS::SECONDS;


uint16_t queueIndex = 0;
etl::vector<QueueEntry, eduProgramQueueSize> eduProgramQueue =
    etl::vector<QueueEntry, eduProgramQueueSize>();
RODOS::RingBuffer<StatusHistoryEntry, statusHistorySize> statusHistory =
    RODOS::RingBuffer<StatusHistoryEntry, statusHistorySize>();


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

            // All variables of the form *Time are in Rodos Time seconds (n of seconds since 1st
            // January 2000).
            auto nextProgramStartTime =
                eduProgramQueue[queueIndex].startTime - utility::rodosUnixOffsetDelay;
            auto currentUtcTime = RODOS::sysTime.getUTC() / RODOS::SECONDS;
            auto const startDelay =
                std::max((nextProgramStartTime - currentUtcTime) * SECONDS, 0 * SECONDS);

            RODOS::PRINTF("Next program will start in  : %" PRIi64 " nanoseconds\n", startDelay);

            // Suspend until delay time - 2 seconds
            RODOS::PRINTF("Suspending for %" PRIi64 " nanoseconds\n",
                          startDelay - eduCommunicationDelay);
            AT(NOW() + startDelay - eduCommunicationDelay);

            RODOS::PRINTF("Resuming here after first start delay");

            // Send UTC to EDU
            auto updateTimeData = periphery::UpdateTimeData{.timestamp = utility::GetUnixUtc()};
            auto errorCode = edu.UpdateTime(updateTimeData);


            // delay again
            
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
