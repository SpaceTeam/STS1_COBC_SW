#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/Enums.hpp>
#include <Sts1CobcSw/Edu/ProgramQueue.hpp>
#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>
#include <Sts1CobcSw/Edu/Structs.hpp>
#include <Sts1CobcSw/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/ThreadPriorities.hpp>
#include <Sts1CobcSw/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>

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


class EduProgramQueueThread : public RODOS::StaticThread<stackSize>
{
public:
    EduProgramQueueThread() : StaticThread("EduProgramQueueThread", eduProgramQueueThreadPriority)
    {
    }


private:
    void init() override
    {
        edu::Initialize();

        // auto queueEntry1 = EduQueueEntry{
        //    .programId = 0, .queueId = 1, .startTime = 946'684'807, .timeout = 10};  // NOLINT

        // auto queueEntry2 = EduQueueEntry{
        //    .programId = 0, .queueId = 2, .startTime = 946'684'820, .timeout = 20};  // NOLINT

        // eduProgramQueue.push_back(queueEntry1);
        // eduProgramQueue.push_back(queueEntry2);

        RODOS::PRINTF("Size of EduProgramQueue : %d\n", edu::programQueue.size());
    }

    void run() override
    {
        // TODO: Define some DebugPrint() or something in a separate file that can be turned on/off
        RODOS::PRINTF("Entering EduProgramQueueThread\n");
        utility::PrintFormattedSystemUtc();
        while(true)
        {
            if(edu::programQueue.empty())
            {
                RODOS::PRINTF(
                    "Edu Program Queue is empty, thread set to sleep until end of time\n");
                AT(RODOS::END_OF_TIME);
            }
            else if(edu::queueIndex >= edu::programQueue.size())
            {
                RODOS::PRINTF("End of queue is reached, thread set to sleep until end of time\n");
                AT(RODOS::END_OF_TIME);
            }

            // All variables in this thread whose name is of the form *Time are in Rodos Time
            // seconds (n of seconds since 1st January 2000).
            auto startDelay = ComputeStartDelay();
            nextProgramStartDelayTopic.publish(startDelay / RODOS::SECONDS);

            RODOS::PRINTF("Program at queue index %d will start in : %" PRIi64 " s\n",
                          edu::queueIndex,
                          startDelay / RODOS::SECONDS);

            // Suspend until delay time - 2 seconds
            RODOS::PRINTF("Suspending for the first time for      : %" PRIi64 " s\n",
                          (startDelay - eduCommunicationDelay) / RODOS::SECONDS);
            AT(NOW() + startDelay - eduCommunicationDelay);
            // RODOS::AT(nextProgramStartTime * SECONDS - eduCommunicationDelay);

            RODOS::PRINTF("Resuming here after first wait.\n");
            utility::PrintFormattedSystemUtc();

            auto errorCode =
                edu::UpdateTime(edu::UpdateTimeData{.timestamp = utility::GetUnixUtc()});
            if(errorCode != edu::ErrorCode::success)
            {
                RODOS::PRINTF("UpdateTime error code : %d\n", static_cast<int>(errorCode));
                RODOS::PRINTF(
                    "[EduProgramQueueThread] Communication error after call to UpdateTime().\n");
                ResumeEduCommunicationErrorThread();
            }

            auto startDelay2 = ComputeStartDelay();
            nextProgramStartDelayTopic.publish(startDelay2 / RODOS::SECONDS);

            RODOS::PRINTF("Program at queue index %d will start in : %" PRIi64 " s\n",
                          edu::queueIndex,
                          startDelay2 / RODOS::SECONDS);

            // Suspend for delay a second time
            RODOS::PRINTF("Suspending for the second time for     : %" PRIi64 " s\n",
                          startDelay2 / SECONDS);
            RODOS::AT(NOW() + startDelay2);

            // Never reached
            RODOS::PRINTF("Done suspending for the second time\n");

            auto queueId = edu::programQueue[edu::queueIndex].queueId;
            auto programId = edu::programQueue[edu::queueIndex].programId;
            auto timeout = edu::programQueue[edu::queueIndex].timeout;

            RODOS::PRINTF("Executing program %d\n", programId);
            auto executeProgramData = edu::ExecuteProgramData{
                .programId = programId, .queueId = queueId, .timeout = timeout};
            // Start Process
            errorCode = edu::ExecuteProgram(executeProgramData);
            // errorCode = periphery::EduErrorCode::success;

            if(errorCode != edu::ErrorCode::success)
            {
                RODOS::PRINTF(
                    "[EduProgramQueueThread] Communication error after call to "
                    "ExecuteProgram().\n");
                ResumeEduCommunicationErrorThread();
            }
            else
            {
                edu::programStatusHistory.put(
                    edu::ProgramStatusHistoryEntry{.programId = programId,
                                                   .queueId = queueId,
                                                   .status = edu::ProgramStatus::programRunning});

                // Suspend Self for execution time
                auto const executionTime = timeout.get() + eduCommunicationDelay;
                RODOS::PRINTF("Suspending for execution time\n");
                AT(NOW() + executionTime);
                RODOS::PRINTF("Resuming from execution time\n");
                utility::PrintFormattedSystemUtc();

                // Set current Queue ID to next
                edu::queueIndex++;
            }
        }
    }
} eduProgramQueueThread;


//! Compute the delay in nanoseconds before the start of program at current queue index
auto ComputeStartDelay() -> std::int64_t
{
    auto nextProgramStartTime =
        edu::programQueue[edu::queueIndex].startTime.get() - (utility::rodosUnixOffset / SECONDS);
    auto currentUtcTime = RODOS::sysTime.getUTC() / SECONDS;
    std::int64_t const startDelay =
        std::max((nextProgramStartTime - currentUtcTime) * SECONDS, 0 * SECONDS);

    return startDelay;
}

auto ResumeEduProgramQueueThread() -> void
{
    eduProgramQueueThread.resume();
    RODOS::PRINTF("[EduProgramQueueThread] EduProgramQueueThread resumed\n");
}
}
