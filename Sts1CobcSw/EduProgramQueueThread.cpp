#include <Sts1CobcSw/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/Periphery/Enums.hpp>
#include <Sts1CobcSw/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <ringbuffer.h>

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


// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 4'000U;
constexpr auto eduCommunicationDelay = 2 * SECONDS;
constexpr auto threadPriority = 300;

periphery::Edu edu = periphery::Edu();


class EduProgramQueueThread : public RODOS::StaticThread<stackSize>
{
public:
    EduProgramQueueThread() : StaticThread("EduQueueThread", threadPriority)
    {
    }

private:
    void init() override
    {
        edu.Initialize();

        auto queueEntry1 = EduQueueEntry{0_u16, 1_u16, 946'684'807_i32, 10_i16};
        auto queueEntry2 = EduQueueEntry{0_u16, 2_u16, 946'684'820_i32, 10_i16};

        eduProgramQueue.push_back(queueEntry1);
        eduProgramQueue.push_back(queueEntry2);
    }

    void run() override
    {
        RODOS::PRINTF("Entering EduQueueThread\n");
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
            auto nextProgramStartTime = eduProgramQueue[queueIndex].startTime.get()
                                      - (utility::rodosUnixOffset / RODOS::SECONDS);
            auto currentUtcTime = RODOS::sysTime.getUTC() / SECONDS;
            auto startDelay =
                std::max((nextProgramStartTime - currentUtcTime) * SECONDS, 0 * SECONDS);
            nextProgramStartDelayTopic.publish(startDelay / RODOS::SECONDS);

            RODOS::PRINTF("Program at queue index %d will start in      : %" PRIi64 " seconds\n",
                          queueIndex,
                          startDelay / RODOS::SECONDS);

            // Suspend until delay time - 2 seconds
            RODOS::PRINTF("Suspending for the first time for            : %" PRIi64 " seconds\n",
                          (startDelay - eduCommunicationDelay) / SECONDS);
            AT(NOW() + startDelay - eduCommunicationDelay);
            // RODOS::AT(nextProgramStartTime * SECONDS - eduCommunicationDelay);

            RODOS::PRINTF("Resuming here after first wait.\n");
            utility::PrintFormattedSystemUtc();

            auto updateTimeData = periphery::UpdateTimeData{.timestamp = utility::GetUnixUtc()};
            auto errorCode = edu.UpdateTime(updateTimeData);
            if(errorCode != periphery::EduErrorCode::success)
            {
                ResumeEduErrorCommunicationThread();
            }

            // TODO: Get rid of the code duplication here
            nextProgramStartTime = eduProgramQueue[queueIndex].startTime.get()
                                 - (utility::rodosUnixOffset / RODOS::SECONDS);
            currentUtcTime = RODOS::sysTime.getUTC() / SECONDS;
            auto startDelay2 =
                std::max((nextProgramStartTime - currentUtcTime) * SECONDS, 0 * SECONDS);
            nextProgramStartDelayTopic.publish(startDelay2 / RODOS::SECONDS);

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
            utility::PrintFormattedSystemUtc();

            auto queueId = eduProgramQueue[queueIndex].queueId;
            auto programId = eduProgramQueue[queueIndex].programId;
            auto timeout = eduProgramQueue[queueIndex].timeout;

            auto executeProgramData = periphery::ExecuteProgramData{
                .programId = programId, .queueId = queueId, .timeout = timeout};
            // Start Process
            errorCode = edu.ExecuteProgram(executeProgramData);

            if(errorCode != periphery::EduErrorCode::success)
            {
                ResumeEduErrorCommunicationThread();
            }
            else
            {
                auto statusHistoryEntry =
                    StatusHistoryEntry{.programId = programId,
                                       .queueId = queueId,
                                       .status = ProgramStatus::programRunning};
                statusHistory.put(statusHistoryEntry);

                // Suspend Self for execution time
                auto const executionTime = timeout.get() + eduCommunicationDelay;
                RODOS::PRINTF("Suspending for execution time\n");
                AT(NOW() + executionTime);
                RODOS::PRINTF("Resuming from execution time\n");
                utility::PrintFormattedSystemUtc();

                // Set current Queue ID to next
                queueIndex++;
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
