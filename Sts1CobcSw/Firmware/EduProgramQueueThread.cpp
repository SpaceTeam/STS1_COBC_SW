#include <Sts1CobcSw/Firmware/EduProgramQueueThread.hpp>

#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/ProgramQueue.hpp>
#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>
#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/Firmware/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/FramRingArray.hpp>
#include <Sts1CobcSw/FramSections/FramVector.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/RealTime/RealTime.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/ProgramId.hpp>  // IWYU pragma: keep
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <cinttypes>  // IWYU pragma: keep
#include <utility>


namespace sts1cobcsw
{
namespace
{
[[nodiscard]] auto ComputeStartDelay(RealTime startTime) -> Duration;


// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 8000U;
constexpr auto eduCommunicationDelay = 2 * s;


class EduProgramQueueThread : public RODOS::StaticThread<stackSize>
{
public:
    EduProgramQueueThread() : StaticThread("EduProgramQueueThread", eduProgramQueueThreadPriority)
    {}


private:
    void init() override
    {
        edu::Initialize();
    }

    void run() override
    {
        while(true)
        {
            if(edu::programQueue.IsEmpty())
            {
                (void)0;  // Silence warning about repeated branch in conditional
                DEBUG_PRINT("Edu Program Queue is empty, thread set to sleep until end of time\n");
                SuspendUntil(endOfTime);
                continue;
            }
            else if(persistentVariables.Load<"eduProgramQueueIndex">() >= edu::programQueue.Size())
            {
                DEBUG_PRINT("End of queue is reached, thread set to sleep until end of time\n");
                SuspendUntil(endOfTime);
                continue;
            }

            auto queueIndex = persistentVariables.Load<"eduProgramQueueIndex">();
            if(queueIndex == -1)
            {
                queueIndex = 0;
            }


            auto queueEntry = edu::programQueue.Get(queueIndex);
            auto startDelay = ComputeStartDelay(queueEntry.startTime);
            nextProgramStartDelayTopic.publish(startDelay);

            // Suspend until delay time - 2 seconds
            SuspendFor(startDelay - eduCommunicationDelay);


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

            // Before suspending a second time, we check if startDelay2 < eduCommunication because that would
            // mean that we loaded a new and different entry compared to the one loaded at the beginning  of the processing queue.
            // If that is the case, we jump back to the top of the loop. 
            if(startDelay2 < eduCommunicationDelay)
            {
                continue;
            }

            SuspendFor(startDelay2);


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
                SuspendFor(executionTime);

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
}


auto ResumeEduProgramQueueThread() -> void
{
    eduProgramQueueThread.resume();
    DEBUG_PRINT("[EduProgramQueueThread] EduProgramQueueThread resumed\n");
}
}
