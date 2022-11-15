#include <Sts1CobcSw/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/LoopedEduProgramQueue.hpp>
#include <Sts1CobcSw/Periphery/EduEnums.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <ringbuffer.h>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <algorithm>
#include <cinttypes>


namespace sts1cobcsw
{
using serial::Byte;

using RODOS::AT;
using RODOS::NOW;
using RODOS::SECONDS;


// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 8'000U;
constexpr auto eduCommunicationDelay = 2 * SECONDS;
constexpr auto threadPriority = 300;

periphery::Edu edu = periphery::Edu();


// TODO: File and class name should match. More generally, consistently call it EduProgramQueue or
// just EduQueue everywhere.
class EduQueueThread : public RODOS::StaticThread<stackSize>
{
public:
    EduQueueThread() : StaticThread("EduQueueThread", threadPriority)
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
        utility::PrintTime();
        // TODO: Define some DebugPrint() or something in a separate file that can be turned on/off
        RODOS::PRINTF("Entering EduQueueThread\n");
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
                eduProgramQueue[queueIndex].startTime - (utility::rodosUnixOffset / RODOS::SECONDS);
            auto currentUtcTime = RODOS::sysTime.getUTC() / SECONDS;
            auto startDelay =
                std::max((nextProgramStartTime - currentUtcTime) * SECONDS, 0 * SECONDS);
            nextProgramStartDelayTopic.publish(startDelay / RODOS::SECONDS);

            RODOS::PRINTF("Program at queue index %d will start in %" PRIi64 " s\n",
                          queueIndex,
                          startDelay / SECONDS);

            AT(NOW() + startDelay - eduCommunicationDelay);

            auto updateTimeData = periphery::UpdateTimeData{.timestamp = utility::GetUnixUtc()};
            auto errorCode = edu.UpdateTime(updateTimeData);
            if(errorCode != periphery::EduErrorCode::success)
            {
                RODOS::PRINTF(
                    "[EduProgramQueueThread] Communication error after call to UpdateTime()\n");
                RODOS::PRINTF("  Returned error code = %d\n", errorCode);
                ResumeEduErrorCommunicationThread();
            }

            // TODO: Get rid of the code duplication here
            nextProgramStartTime =
                eduProgramQueue[queueIndex].startTime - (utility::rodosUnixOffset / RODOS::SECONDS);
            currentUtcTime = RODOS::sysTime.getUTC() / SECONDS;
            auto startDelay2 =
                std::max((nextProgramStartTime - currentUtcTime) * SECONDS, 0 * SECONDS);
            nextProgramStartDelayTopic.publish(startDelay2 / RODOS::SECONDS);

            RODOS::PRINTF("Program at queue index %d will start in : %" PRIi64 " s\n",
                          queueIndex,
                          startDelay2 / RODOS::SECONDS);

            RODOS::AT(NOW() + startDelay2);

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
                statusHistory.put({.programId = programId, .queueId = queueId, .status = 1});

                AT(NOW() + timeout + eduCommunicationDelay);

                // Loop EDU program queue
                UpdateEduProgramQueueEntry(&eduProgramQueue[queueIndex]);
                UpdateQueueIndex();
            }
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
