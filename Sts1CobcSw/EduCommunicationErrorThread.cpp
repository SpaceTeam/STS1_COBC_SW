#include <Sts1CobcSw/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/ThreadPriorities.hpp>
#include <Sts1CobcSw/TopicsAndSubscribers.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
constexpr auto stackSize = 2'000U;
constexpr auto eduShutDownDelay = 2 * RODOS::SECONDS;
std::int32_t eduCommunicationErrorCounter = 0;


class EduCommunicationErrorThread : public RODOS::StaticThread<stackSize>
{
public:
    EduCommunicationErrorThread()
        : StaticThread("EduCommunicationThread", eduCommunicationErrorThreadPriority)
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
            periphery::edu.TurnOff();
            RODOS::AT(RODOS::NOW() + eduShutDownDelay);
            periphery::edu.TurnOn();

            // TODO: Why is this here?
            [[maybe_unused]] auto status = periphery::edu.GetStatus();

            // Busy wait
            RODOS::PRINTF("[EduCommunicationErrorThread] Entering busy wait\n");
            auto eduIsAlive = false;
            while(not eduIsAlive)
            {
                yield();  // Force recalculation of scheduling!
                eduIsAliveBufferForCommunicationError.get(eduIsAlive);
            }
            RODOS::PRINTF("[EduCommunicationErrorThread] Leaving busy wait\n");
        }
    }
} eduCommunicationErrorThread;


auto ResumeEduCommunicationErrorThread() -> void
{
    eduCommunicationErrorThread.resume();
    RODOS::PRINTF("[EduCommunicationErrorThread] EduCommunicationErrorThread resumed\n");
}
}
