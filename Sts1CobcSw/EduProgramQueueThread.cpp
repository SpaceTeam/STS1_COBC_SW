#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/ProgramQueue.hpp>
#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>
#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/ProgramId/ProgramId.hpp>  // IWYU pragma: keep
#include <Sts1CobcSw/ThreadPriorities.hpp>
#include <Sts1CobcSw/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/Utility/Debug.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <strong_type/type.hpp>

#include <rodos/support/support-libs/ringbuffer.h>
#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <algorithm>
#include <cinttypes>  // IWYU pragma: keep
#include <cstdint>


namespace sts1cobcsw
{
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
        //    .programId = 0, .timestamp = 1, .startTime = 946'684'807, .timeout = 10};  // NOLINT

        // auto queueEntry2 = EduQueueEntry{
        //    .programId = 0, .timestamp = 2, .startTime = 946'684'820, .timeout = 20};  // NOLINT

        // eduProgramQueue.push_back(queueEntry1);
        // eduProgramQueue.push_back(queueEntry2);

        DEBUG_PRINT("Size of EduProgramQueue : %zu\n", edu::programQueue.size());
    }

    void run() override
    {
        // TODO: Define some DebugPrint() or something in a separate file that can be turned on/off
        DEBUG_PRINT("Entering EduProgramQueueThread\n");
        utility::PrintFormattedSystemUtc();
        while(true)
        {
            if(edu::programQueue.empty())
            {
                DEBUG_PRINT("Edu Program Queue is empty, thread set to sleep until end of time\n");
                AT(RODOS::END_OF_TIME);
            }
            else if(edu::queueIndex >= edu::programQueue.size())
            {
                DEBUG_PRINT("End of queue is reached, thread set to sleep until end of time\n");
                AT(RODOS::END_OF_TIME);
            }

            // All variables in this thread whose name is of the form *Time are in Rodos Time
            // seconds (n of seconds since 1st January 2000).
            auto startDelay = ComputeStartDelay();
            nextProgramStartDelayTopic.publish(startDelay / RODOS::SECONDS);

            DEBUG_PRINT("Program at queue index %d will start in : %" PRIi64 " s\n",
                        edu::queueIndex,
                        startDelay / RODOS::SECONDS);

            // Suspend until delay time - 2 seconds
            DEBUG_PRINT("Suspending for the first time for      : %" PRIi64 " s\n",
                        (startDelay - eduCommunicationDelay) / RODOS::SECONDS);
            AT(NOW() + startDelay - eduCommunicationDelay);
            // RODOS::AT(nextProgramStartTime * SECONDS - eduCommunicationDelay);

            DEBUG_PRINT("Resuming here after first wait.\n");
            utility::PrintFormattedSystemUtc();

            auto updateTimeResult =
                edu::UpdateTime(edu::UpdateTimeData{.currentTime = utility::GetUnixUtc()});
            if(updateTimeResult.has_error())
            {
                DEBUG_PRINT("UpdateTime error code : %d\n",
                            static_cast<int>(updateTimeResult.error()));
                DEBUG_PRINT(
                    "[EduProgramQueueThread] Communication error after call to UpdateTime().\n");
                ResumeEduCommunicationErrorThread();
            }

            auto startDelay2 = ComputeStartDelay();
            nextProgramStartDelayTopic.publish(startDelay2 / RODOS::SECONDS);

            DEBUG_PRINT("Program at queue index %d will start in : %" PRIi64 " s\n",
                        edu::queueIndex,
                        startDelay2 / RODOS::SECONDS);

            // Suspend for delay a second time
            DEBUG_PRINT("Suspending for the second time for     : %" PRIi64 " s\n",
                        startDelay2 / SECONDS);
            RODOS::AT(NOW() + startDelay2);

            // Never reached
            DEBUG_PRINT("Done suspending for the second time\n");

            auto startTime = edu::programQueue[edu::queueIndex].startTime;
            auto programId = edu::programQueue[edu::queueIndex].programId;
            auto timeout = edu::programQueue[edu::queueIndex].timeout;

            DEBUG_PRINT("Executing program %" PRIu16 "\n", value_of(programId));
            auto executeProgramData = edu::ExecuteProgramData{
                .programId = programId, .startTime = startTime, .timeout = timeout};
            // Start Process
            auto executeProgramResult = edu::ExecuteProgram(executeProgramData);
            // errorCode = edu::ErrorCode::success;

            if(executeProgramResult.has_error())
            {
                DEBUG_PRINT(
                    "[EduProgramQueueThread] Communication error after call to "
                    "ExecuteProgram().\n");
                ResumeEduCommunicationErrorThread();
            }
            else
            {
                edu::programStatusHistory.put(
                    edu::ProgramStatusHistoryEntry{.programId = programId,
                                                   .startTime = startTime,
                                                   .status = edu::ProgramStatus::programRunning});

                // Suspend Self for execution time
                auto const executionTime = timeout + eduCommunicationDelay;
                DEBUG_PRINT("Suspending for execution time\n");
                AT(NOW() + executionTime);
                DEBUG_PRINT("Resuming from execution time\n");
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
        edu::programQueue[edu::queueIndex].startTime - (utility::rodosUnixOffset / SECONDS);
    auto currentUtcTime = RODOS::sysTime.getUTC() / SECONDS;
    std::int64_t const startDelay =
        std::max((nextProgramStartTime - currentUtcTime) * SECONDS, 0 * SECONDS);

    return startDelay;
}

auto ResumeEduProgramQueueThread() -> void
{
    eduProgramQueueThread.resume();
    DEBUG_PRINT("[EduProgramQueueThread] EduProgramQueueThread resumed\n");
}
}
