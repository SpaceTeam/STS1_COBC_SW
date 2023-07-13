#include <Sts1CobcSw/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/EduProgramStatusHistory.hpp>
#include <Sts1CobcSw/LoopedEduProgramQueue.hpp>
#include <Sts1CobcSw/Periphery/EduEnums.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/ThreadPriorities.hpp>
#include <Sts1CobcSw/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <rodos/support/support-libs/ringbuffer.h>
#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <algorithm>
#include <cinttypes>


namespace sts1cobcsw
{
namespace ts = type_safe;
using serial::Byte;

using ts::operator""_i16;
using ts::operator""_u16;
using ts::operator""_i32;

using RODOS::AT;
using RODOS::NOW;
using RODOS::SECONDS;


[[nodiscard]] auto ComputeStartDelay() -> std::int64_t;


// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 8'000U;
constexpr auto eduCommunicationDelay = 2 * SECONDS;

periphery::Edu edu = periphery::Edu();


class EduProgramQueueThread : public RODOS::StaticThread<stackSize>
{
public:
    EduProgramQueueThread() : StaticThread("EduProgramQueueThread", eduProgramQueueThreadPriority)
    {
    }

private:
    void init() override
    {
        edu.Initialize();
        InitializeEduProgramQueue();

        RODOS::PRINTF("Size of EduProgramQueue : %d\n", eduProgramQueue.size());
    }

    void run() override
    {
        // TODO: Define some DebugPrint() or something in a separate file that can be turned on/off
        utility::PrintSeconds();
        RODOS::PRINTF("Entering EduProgramQueueThread\n");
        utility::PrintFormattedSystemUtc();
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
            auto startDelay = ComputeStartDelay();
            nextProgramStartDelayTopic.publish(startDelay / RODOS::SECONDS);

            utility::PrintSeconds();
            RODOS::PRINTF("Program at queue index %d will start in %" PRIi64 " s\n",
                          queueIndex,
                          startDelay / SECONDS);

            AT(NOW() + startDelay - eduCommunicationDelay);
            // RODOS::AT(nextProgramStartTime * SECONDS - eduCommunicationDelay);

            // RODOS::PRINTF("Resuming here after first wait.\n");
            // utility::PrintFormattedSystemUtc();

            auto updateTimeData = periphery::UpdateTimeData{.timestamp = utility::GetUnixUtc()};
            auto errorCode = edu.UpdateTime(updateTimeData);
            if(errorCode != periphery::EduErrorCode::success)
            {
                RODOS::PRINTF(
                    "[EduProgramQueueThread] Communication error after call to UpdateTime()\n");
                RODOS::PRINTF("  Returned error code = %d\n", errorCode);
                ResumeEduErrorCommunicationThread();
            }

            auto startDelay2 = ComputeStartDelay();
            nextProgramStartDelayTopic.publish(startDelay2 / RODOS::SECONDS);

            utility::PrintSeconds();
            RODOS::PRINTF("Program at queue index %d will start in : %" PRIi64 " s\n",
                          queueIndex,
                          startDelay2 / RODOS::SECONDS);

            RODOS::AT(NOW() + startDelay2);

            // Never reached
            // RODOS::PRINTF("Done suspending for the second time\n");

            auto queueId = eduProgramQueue[queueIndex].queueId;
            auto programId = eduProgramQueue[queueIndex].programId;
            auto timeout = eduProgramQueue[queueIndex].timeout;

            errorCode = edu.ExecuteProgram(
                {.programId = programId, .queueId = queueId, .timeout = timeout});
            if(errorCode != periphery::EduErrorCode::success)
            {
                // TODO: Think about what is right to do here. As it is right now queueIndex is
                // never updated so we retry executing the same EDU program forever.
                RODOS::PRINTF(
                    "[EduProgramQueueThread] Communication error after call to "
                    "ExecuteProgram()\n");
                RODOS::PRINTF("  Returned error code = %d\n", errorCode);
                ResumeEduErrorCommunicationThread();
            }
            else
            {
                eduProgramStatusHistory.put(
                    EduProgramStatusHistoryEntry{.programId = programId,
                                                 .queueId = queueId,
                                                 .status = EduProgramStatus::programRunning});

                // Suspend Self for execution time
                auto const executionTime = timeout.get() + eduCommunicationDelay;
                utility::PrintSeconds();
                RODOS::PRINTF("Suspending for execution time\n");
                AT(NOW() + executionTime);
                utility::PrintSeconds();
                RODOS::PRINTF("Resuming from execution time\n");
                utility::PrintFormattedSystemUtc();

                // Loop EDU program queue
                utility::PrintSeconds();
                RODOS::PRINTF("\n\nUpdating queue entry for next iteration\n\n");
                UpdateEduProgramQueueEntry(&eduProgramQueue[queueIndex]);
                UpdateQueueIndex();
            }
        }
    }
} eduProgramQueueThread;


//! Compute the delay in nanoseconds before the start of program at current queue index
auto ComputeStartDelay() -> std::int64_t
{
    auto nextProgramStartTime =
        eduProgramQueue[queueIndex].startTime.get() - (utility::rodosUnixOffset / SECONDS);
    auto currentUtcTime = RODOS::sysTime.getUTC() / SECONDS;
    std::int64_t startDelay =
        std::max((nextProgramStartTime - currentUtcTime) * SECONDS, 0 * SECONDS);

    return startDelay;
}


// TODO: Think about whether this is the right way to declare, design, use, etc. this
class ResumeEduProgramQueueThreadEvent : public RODOS::TimeEvent
{
public:
    auto handle() -> void override
    {
        eduProgramQueueThread.resume();
        utility::PrintSeconds();
        RODOS::PRINTF("EduProgramQueueThread resumed from me\n");
    }
} resumeEduProgramQueueThreadEvent;


auto ResumeEduProgramQueueThread() -> void
{
    resumeEduProgramQueueThreadEvent.handle();
}
}
