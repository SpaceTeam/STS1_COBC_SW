#include <Sts1CobcSw/CobcSoftware/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/CobcSoftware/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/CobcSoftware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/ProgramQueue.hpp>
#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>
#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/FramRingArray.hpp>
#include <Sts1CobcSw/FramSections/FramVector.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Utility/RealTime.hpp>
#include <Sts1CobcSw/Utility/RodosTime.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>
#include <Sts1CobcSw/Vocabulary/ProgramId.hpp>  // IWYU pragma: keep

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <cinttypes>  // IWYU pragma: keep


namespace sts1cobcsw
{
[[nodiscard]] auto ComputeStartDelay(RealTime startTime) -> Duration;


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

        DEBUG_PRINT("Size of EDU program queue = %" PRIu32 "\n", edu::programQueue.Size());
    }

    void run() override
    {
        DEBUG_PRINT("Entering EduProgramQueueThread\n");
        DEBUG_PRINT_REAL_TIME();
        while(true)
        {
            if(edu::programQueue.IsEmpty())
            {
                DEBUG_PRINT("Edu Program Queue is empty, thread set to sleep until end of time\n");
                SuspendUntil(endOfTime);
            }
            else if(persistentVariables.Load<"eduProgramQueueIndex">() >= edu::programQueue.Size())
            {
                DEBUG_PRINT("End of queue is reached, thread set to sleep until end of time\n");
                SuspendUntil(endOfTime);
            }

            auto queueIndex = persistentVariables.Load<"eduProgramQueueIndex">();
            auto queueEntry = edu::programQueue.Get(queueIndex);
            auto startDelay = ComputeStartDelay(queueEntry.startTime);
            nextProgramStartDelayTopic.publish(startDelay);

            DEBUG_PRINT("Program at queue index %d will start in : %" PRIi64 " s\n",
                        queueIndex,
                        startDelay / s);

            // Suspend until delay time - 2 seconds
            DEBUG_PRINT("Suspending for the first time for      : %" PRIi64 " s\n",
                        (startDelay - eduCommunicationDelay) / s);
            SuspendFor(startDelay - eduCommunicationDelay);

            DEBUG_PRINT("Resuming here after first wait.\n");
            DEBUG_PRINT_REAL_TIME();

            auto updateTimeResult = edu::UpdateTime({CurrentRealTime()});
            if(updateTimeResult.has_error())
            {
                DEBUG_PRINT("UpdateTime error code : %d\n",
                            static_cast<int>(updateTimeResult.error()));
                DEBUG_PRINT(
                    "[EduProgramQueueThread] Communication error after call to UpdateTime().\n");
                ResumeEduCommunicationErrorThread();
            }

            // Reload queue index and entry because the start delay might be very long and the
            // variables might have been corrupted in the meantime
            queueIndex = persistentVariables.Load<"eduProgramQueueIndex">();
            queueEntry = edu::programQueue.Get(queueIndex);
            auto startDelay2 = ComputeStartDelay(queueEntry.startTime);
            nextProgramStartDelayTopic.publish(startDelay2);

            DEBUG_PRINT("Program at queue index %d will start in : %" PRIi64 " s\n",
                        queueIndex,
                        startDelay2 / s);

            // Suspend for delay a second time
            DEBUG_PRINT("Suspending for the second time for     : %" PRIi64 " s\n",
                        startDelay2 / s);
            SuspendFor(startDelay2);

            // Never reached
            DEBUG_PRINT("Done suspending for the second time\n");

            DEBUG_PRINT("Executing EDU program %" PRIu16 "\n", value_of(queueEntry.programId));
            // Start Process
            auto executeProgramResult = edu::ExecuteProgram(
                {queueEntry.programId, queueEntry.startTime, queueEntry.timeout});
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
                edu::programStatusHistory.PushBack({queueEntry.programId,
                                                    queueEntry.startTime,
                                                    edu::ProgramStatus::programRunning});

                // Suspend Self for execution time
                auto const executionTime = queueEntry.timeout * s + eduCommunicationDelay;
                DEBUG_PRINT("Suspending for execution time\n");
                SuspendFor(executionTime);
                DEBUG_PRINT("Resuming from execution time\n");
                DEBUG_PRINT_REAL_TIME();

                // Set current queue ID to next
                persistentVariables.Increment<"eduProgramQueueIndex">();
            }
        }
    }
} eduProgramQueueThread;


// TODO: We should just use time points instead of delays in the thread. Then we won't need this
// function.
//! Compute the delay in nanoseconds before the start of program at current queue index
auto ComputeStartDelay(RealTime startTime) -> Duration
{
    auto delay = ToRodosTime(startTime) - CurrentRodosTime();
    return delay < Duration(0) ? Duration(0) : delay;
}


auto ResumeEduProgramQueueThread() -> void
{
    eduProgramQueueThread.resume();
    DEBUG_PRINT("[EduProgramQueueThread] EduProgramQueueThread resumed\n");
}
}
