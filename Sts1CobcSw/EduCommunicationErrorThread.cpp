#include <Sts1CobcSw/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/TopicsAndSubscribers.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
constexpr auto stackSize = 2'000U;
constexpr auto threadPriority = 400;
std::int32_t eduCommunicationErrorCounter = 0;


// TODO: Give this thread and the other EDU threads the right priority. Otherwise this concept does
// not work.
class EduCommunicationErrorThread : public RODOS::StaticThread<stackSize>
{
public:
    EduCommunicationErrorThread() : StaticThread("EduCommunicationThread", threadPriority)
    {
    }

private:
    void init() override
    {
    }


    void run() override
    {
        while(true)
        {
            RODOS::AT(RODOS::END_OF_TIME);
            // RODOS::PRINTF("EduCommunicationThread resumed in main loop\n");

            eduCommunicationErrorCounter++;

            RODOS::PRINTF("[EduCommunicationErrorThread] Resetting the Edu\n");
            // Reset EDU
            edu.TurnOff();
            // TODO: Name the 2 seconds
            RODOS::AT(RODOS::NOW() + 2 * RODOS::SECONDS);
            edu.TurnOn();

            //
            [[maybe_unused]] auto status = edu.GetStatus();

            // Busy wait
            RODOS::PRINTF("[EduCommunicationErrorThread] Entering busy wait\n");
            auto eduIsAlive = false;
            while(not eduIsAlive)
            {
                eduIsAliveBufferForCommunicationError.get(eduIsAlive);
            }
            RODOS::PRINTF("[EduCommunicationErrorThread] Leaving busy wait\n");
        }
    }
} eduCommunicationErrorThread;


// TODO: Think about whether this is the right way to declare, design, use, etc. this
class ResumeEduErrorCommunicationThread : public RODOS::TimeEvent
{
public:
    auto handle() -> void override
    {
        eduCommunicationErrorThread.resume();
        RODOS::PRINTF("[EduCommunicationErrorThread] EduCommunicationThread resumed\n");
    }
} resumeEduErrorCommunicationThread;


auto ResumeEduErrorCommunicationThread() -> void
{
    resumeEduErrorCommunicationThread.handle();
}
}
