#include <Sts1CobcSw/Firmware/EduProgramQueueThread.hpp>

#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/ProgramQueue.hpp>
#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>
#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/Firmware/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/Firmware/EduPowerManagementThread.hpp>
#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/FramRingArray.hpp>
#include <Sts1CobcSw/FramSections/FramVector.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RealTime/RealTime.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/ProgramId.hpp>  // IWYU pragma: keep
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <cinttypes>  // IWYU pragma: keep
#include <compare>
#include <utility>


namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 1500U;
// This thread must start before the EDU power management thread to ensure the correct start time
// is published
constexpr auto eduProgramQueueThreadStartDelay = eduPowerManagementThreadStartDelay - 1 * s;
static_assert(eduProgramQueueThreadStartDelay > 0 * s);
constexpr auto eduCommunicationMargin = 2 * s;
constexpr auto maxScheduleDelay = 1 * min;
constexpr auto eduIsAliveCheckInterval = 1 * s;


[[nodiscard]] auto PublishAndConvert(RealTime startTime) -> RodosTime;
[[nodiscard]] auto ProcessQueueEntry() -> Result<void>;
auto SuspendUntilEduIsAlive() -> void;


class EduProgramQueueThread : public RODOS::StaticThread<stackSize>
{
public:
    EduProgramQueueThread() : StaticThread("EduProgramQueueThread", eduProgramQueueThreadPriority)
    {}


private:
    void run() override
    {
        SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
        SuspendFor(eduProgramQueueThreadStartDelay);
        DEBUG_PRINT("Starting EDU program queue thread\n");
        while(true)
        {
            auto queueIndex = persistentVariables.Load<"eduProgramQueueIndex">();
            DEBUG_PRINT("EDU program queue: index = %i, size = %i\n",
                        queueIndex,
                        static_cast<int>(edu::programQueue.Size()));
            if(queueIndex == eduProgramQueueIndexResetValue)
            {
                queueIndex = 0;
                persistentVariables.Store<"eduProgramQueueIndex">(0);
            }
            if(queueIndex >= edu::programQueue.Size())
            {
                DEBUG_PRINT("Reached end of EDU program queue\n");
                // This variable is necessary since publish() does not accept constants
                auto startTime = endOfRealTime;
                nextEduProgramStartTimeTopic.publish(startTime);
                programIdOfCurrentEduProgramQueueEntryTopic.publish(ProgramId(-1));
                SuspendUntil(endOfTime);
                continue;
            }
            auto entry = edu::programQueue.Get(queueIndex);
            auto startTime = PublishAndConvert(entry.startTime);
            programIdOfCurrentEduProgramQueueEntryTopic.publish(entry.programId);
            DEBUG_PRINT_REAL_TIME();
            DEBUG_PRINT("Next EDU queue entry: program ID = %i, start time = %u, timeout = %i s\n",
                        value_of(entry.programId),
                        static_cast<unsigned>(value_of(entry.startTime)),
                        entry.timeout);
            SuspendUntil(startTime - eduCommunicationMargin);
            auto result = ProcessQueueEntry();
            if(result.has_error() and result.error() == ErrorCode::eduIsNotAlive)
            {
                DEBUG_PRINT("EDU is not alive, suspending until it's back\n");
                SuspendUntilEduIsAlive();
            }
            else if(result.has_error())
            {
                DEBUG_PRINT("Failed to process EDU program queue entry: %s\n",
                            ToCZString(result.error()));
                ResumeEduCommunicationErrorThread();
            }
            DEBUG_PRINT_STACK_USAGE();
        }
    }
} eduProgramQueueThread;
}


auto ResumeEduProgramQueueThread() -> void
{
    eduProgramQueueThread.resume();
}


namespace
{
auto PublishAndConvert(RealTime startTime) -> RodosTime
{
    nextEduProgramStartTimeTopic.publish(startTime);
    return ToRodosTime(startTime);
}


auto ProcessQueueEntry() -> Result<void>
{
    auto queueIndex = persistentVariables.Load<"eduProgramQueueIndex">();
    if(queueIndex == eduProgramQueueIndexResetValue)
    {
        return outcome_v2::success();
    }
    auto queueEntry = edu::programQueue.Get(queueIndex);
    auto startTime = PublishAndConvert(queueEntry.startTime);
    DEBUG_PRINT("Updating EDU time to %u\n", static_cast<unsigned>(value_of(CurrentRealTime())));
    auto eduIsAlive = false;
    eduIsAliveBufferForProgramQueue.get(eduIsAlive);
    if(not eduIsAlive)
    {
        return ErrorCode::eduIsNotAlive;
    }
    OUTCOME_TRY(edu::UpdateTime({CurrentRealTime()}));
    SuspendUntil(startTime);

    if(persistentVariables.Load<"eduProgramQueueIndex">() == eduProgramQueueIndexResetValue)
    {
        return outcome_v2::success();
    }
    if(CurrentRodosTime() > startTime + maxScheduleDelay)
    {
        DEBUG_PRINT("Skipping EDU program %i because it is too late\n",
                    static_cast<int>(value_of(queueEntry.programId)));
        persistentVariables.Increment<"eduProgramQueueIndex">();
        return outcome_v2::success();
    }
    DEBUG_PRINT("Executing EDU program: ID = %i, start time = %u, timeout = %i s\n",
                static_cast<int>(value_of(queueEntry.programId)),
                static_cast<unsigned>(value_of(queueEntry.startTime)),
                static_cast<int>(queueEntry.timeout));
    eduIsAliveBufferForProgramQueue.get(eduIsAlive);
    if(not eduIsAlive)
    {
        return ErrorCode::eduIsNotAlive;
    }
    OUTCOME_TRY(
        edu::ExecuteProgram({queueEntry.programId, queueEntry.startTime, queueEntry.timeout}));
    edu::programStatusHistory.PushBack(
        {queueEntry.programId, queueEntry.startTime, edu::ProgramStatus::programRunning});
    SuspendFor(queueEntry.timeout * s + eduCommunicationMargin);
    if(persistentVariables.Load<"eduProgramQueueIndex">() == eduProgramQueueIndexResetValue)
    {
        return outcome_v2::success();
    }
    persistentVariables.Increment<"eduProgramQueueIndex">();
    return outcome_v2::success();
}


auto SuspendUntilEduIsAlive() -> void
{
    while(true)
    {
        auto eduIsAlive = false;
        eduIsAliveBufferForProgramQueue.get(eduIsAlive);
        if(eduIsAlive)
        {
            return;
        }
        SuspendFor(eduIsAliveCheckInterval);
    }
}
}
}
