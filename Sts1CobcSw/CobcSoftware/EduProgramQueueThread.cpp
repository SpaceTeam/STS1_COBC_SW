#include <Sts1CobcSw/CobcSoftware/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/CobcSoftware/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/CobcSoftware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/ProgramQueue.hpp>
#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>
#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/FramSections/RingArray.hpp>
#include <Sts1CobcSw/ProgramId/ProgramId.hpp>  // IWYU pragma: keep
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Utility/RealTime.hpp>
#include <Sts1CobcSw/Utility/RodosTime.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <cinttypes>  // IWYU pragma: keep


namespace sts1cobcsw
{
[[nodiscard]] auto ComputeStartDelay() -> Duration;


// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 8'000U;
constexpr auto eduCommunicationDelay = 2 * s;


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
        DEBUG_PRINT_REAL_TIME();
        while(true)
        {
            if(edu::programQueue.empty())
            {
                DEBUG_PRINT("Edu Program Queue is empty, thread set to sleep until end of time\n");
                SuspendUntil(endOfTime);
            }
            else if(edu::queueIndex >= edu::programQueue.size())
            {
                DEBUG_PRINT("End of queue is reached, thread set to sleep until end of time\n");
                SuspendUntil(endOfTime);
            }

            auto startDelay = ComputeStartDelay();
            nextProgramStartDelayTopic.publish(startDelay);

            DEBUG_PRINT("Program at queue index %d will start in : %" PRIi64 " s\n",
                        edu::queueIndex,
                        startDelay / s);

            // Suspend until delay time - 2 seconds
            DEBUG_PRINT("Suspending for the first time for      : %" PRIi64 " s\n",
                        (startDelay - eduCommunicationDelay) / s);
            SuspendFor(startDelay - eduCommunicationDelay);

            DEBUG_PRINT("Resuming here after first wait.\n");
            DEBUG_PRINT_REAL_TIME();

            auto updateTimeResult =
                edu::UpdateTime(edu::UpdateTimeData{.currentTime = CurrentRealTime()});
            if(updateTimeResult.has_error())
            {
                DEBUG_PRINT("UpdateTime error code : %d\n",
                            static_cast<int>(updateTimeResult.error()));
                DEBUG_PRINT(
                    "[EduProgramQueueThread] Communication error after call to UpdateTime().\n");
                ResumeEduCommunicationErrorThread();
            }

            auto startDelay2 = ComputeStartDelay();
            nextProgramStartDelayTopic.publish(startDelay2);

            DEBUG_PRINT("Program at queue index %d will start in : %" PRIi64 " s\n",
                        edu::queueIndex,
                        startDelay2 / s);

            // Suspend for delay a second time
            DEBUG_PRINT("Suspending for the second time for     : %" PRIi64 " s\n",
                        startDelay2 / s);
            SuspendFor(startDelay2);

            // Never reached
            DEBUG_PRINT("Done suspending for the second time\n");

            auto startTime = edu::programQueue[edu::queueIndex].startTime;
            auto programId = edu::programQueue[edu::queueIndex].programId;
            auto timeout = edu::programQueue[edu::queueIndex].timeout;

            DEBUG_PRINT("Executing program %" PRIu16 "\n", value_of(programId));
            // Start Process
            auto executeProgramResult = edu::ExecuteProgram(edu::ExecuteProgramData{
                .programId = programId, .startTime = startTime, .timeout = timeout});
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
                edu::programStatusHistory.PushBack(
                    edu::ProgramStatusHistoryEntry{.programId = programId,
                                                   .startTime = startTime,
                                                   .status = edu::ProgramStatus::programRunning});

                // Suspend Self for execution time
                auto const executionTime = timeout * s + eduCommunicationDelay;
                DEBUG_PRINT("Suspending for execution time\n");
                SuspendFor(executionTime);
                DEBUG_PRINT("Resuming from execution time\n");
                DEBUG_PRINT_REAL_TIME();

                // Set current Queue ID to next
                edu::queueIndex++;
            }
        }
    }
} eduProgramQueueThread;


// TODO: We should just use time points instead of delays in the thread. Then we won't need this
// function.
//! Compute the delay in nanoseconds before the start of program at current queue index
auto ComputeStartDelay() -> Duration
{
    auto delay = ToRodosTime(edu::programQueue[edu::queueIndex].startTime) - CurrentRodosTime();
    return delay < Duration(0) ? Duration(0) : delay;
}


auto ResumeEduProgramQueueThread() -> void
{
    eduProgramQueueThread.resume();
    DEBUG_PRINT("[EduProgramQueueThread] EduProgramQueueThread resumed\n");
}
}
