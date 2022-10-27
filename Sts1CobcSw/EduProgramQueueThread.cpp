#include <Sts1CobcSw/EduProgramQueue.hpp>
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


namespace sts1cobcsw
{
using sts1cobcsw::serial::Byte;

using RODOS::AT;
using RODOS::NOW;
using RODOS::SECONDS;


auto edu = periphery::Edu();
constexpr auto eduCommunicationDelay = 2 * SECONDS;


// TODO: File and class name should match. More generally, consistently call it EduProgramQueue or
// just EduQueue everywhere.
class EduQueueThread : public RODOS::StaticThread<>
{
    void init() override
    {
        auto queueEntry1 = EduQueueEntry{
            .programId = 5, .queueId = 1, .startTime = 1672531215, .timeout = 10};  // NOLINT
        AddQueueEntry(queueEntry1);

        auto queueEntry2 = EduQueueEntry{
            .programId = 6, .queueId = 1, .startTime = 1672531230, .timeout = 20};  // NOLINT

        AddQueueEntry(queueEntry1);
        AddQueueEntry(queueEntry2);
    }

    void run() override
    {
        RODOS::PRINTF("Entering EduQueueThread\n");
        utility::PrintTime();
        while(true)
        {
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

            // All variables in this thread whose name is of the form *Time are in Rodos Time
            // seconds (n of seconds since 1st January 2000).
            auto nextProgramStartTime =
                eduProgramQueue[queueIndex].startTime - utility::rodosUnixOffset;
            auto currentUtcTime = RODOS::sysTime.getUTC() / SECONDS;
            auto const startDelay =
                std::max((nextProgramStartTime - currentUtcTime) * SECONDS, 0 * SECONDS);

            RODOS::PRINTF("Program at queue index %d will start in      : %" PRIi64 " seconds\n",
                          queueIndex,
                          startDelay / RODOS::SECONDS);

            // Suspend until delay time - 2 seconds
            RODOS::PRINTF("Suspending for the first time for            : %" PRIi64 " seconds\n",
                          (startDelay - eduCommunicationDelay) / SECONDS);
            AT(NOW() + startDelay - eduCommunicationDelay);
            // RODOS::AT(nextProgramStartTime * SECONDS - eduCommunicationDelay);

            RODOS::PRINTF("Resuming here after first wait.\n");
            utility::PrintTime();

            auto updateTimeData = periphery::UpdateTimeData{.timestamp = utility::GetUnixUtc()};
            // TODO: Do something with error code
            [[maybe_unused]] auto errorCode = edu.UpdateTime(updateTimeData);

            nextProgramStartTime =
                eduProgramQueue[queueIndex].startTime - utility::rodosUnixOffset;
            currentUtcTime = RODOS::sysTime.getUTC() / SECONDS;
            auto const startDelay2 =
                std::max((nextProgramStartTime - currentUtcTime) * SECONDS, 0 * SECONDS);

            RODOS::PRINTF("Program at queue index %d will start in      : %" PRIi64 " seconds\n",
                          queueIndex,
                          startDelay2 / RODOS::SECONDS);

            // Suspend for delay a second time
            RODOS::PRINTF("Suspending for the second time for            : %" PRIi64 " seconds\n",
                          startDelay2 / SECONDS);
            auto const begin = RODOS::NOW();
            RODOS::AT(NOW() + startDelay2);
            auto end = RODOS::NOW() - begin;
            RODOS::PRINTF("Done suspending, suspended for                :%lld\n",
                          end / RODOS::SECONDS);  // NOLINT
            utility::PrintTime();

            auto queueId = eduProgramQueue[queueIndex].queueId;
            auto programId = eduProgramQueue[queueIndex].programId;
            auto timeout = eduProgramQueue[queueIndex].timeout;

            auto executeProgramData = periphery::ExecuteProgramData{
                .programId = programId, .queueId = queueId, .timeout = timeout};
            // Start Process
            auto eduAnswer = edu.ExecuteProgram(executeProgramData);

            // Suspend Self for execution time
            auto const executionTime = timeout + eduCommunicationDelay;
            RODOS::PRINTF("Suspending for execution time\n");
            AT(NOW() + executionTime);
            RODOS::PRINTF("Resuming from execution time\n");
            utility::PrintTime();

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
} eduQueueThread;


// TODO: Think about whether this is the right way to declare, design, use, etc. this
class ResumeEduQueueThreadEvent : public RODOS::TimeEvent
{
public:
    auto handle() -> void override
    {
        eduQueueThread.resume();
        RODOS::PRINTF("EduQueueThread resumed from me\n");
    }
} resumeEduQueueThreadEvent;


auto ResumeEduQueueThread() -> void
{
    resumeEduQueueThreadEvent.handle();
}
}
