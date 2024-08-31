#include <Sts1CobcSw/CobcSoftware/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/CobcSoftware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/Utility/Debug.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <strong_type/difference.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
constexpr auto stackSize = 2'000U;
constexpr auto eduShutDownDelay = 2 * s;


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
            SuspendUntil(endOfTime);

            persistentVariables.template Increment<"eduCommunicationErrorCounter">();

            DEBUG_PRINT("[EduCommunicationErrorThread] Resetting the Edu\n");
            // Reset EDU

            edu::TurnOff();
            SuspendFor(eduShutDownDelay);
            edu::TurnOn();

            // TODO: Why is this here?
            [[maybe_unused]] auto status = edu::GetStatus();

            // Busy wait
            DEBUG_PRINT("[EduCommunicationErrorThread] Entering busy wait\n");
            auto eduIsAlive = false;
            while(not eduIsAlive)
            {
                yield();  // Force recalculation of scheduling!
                eduIsAliveBufferForCommunicationError.get(eduIsAlive);
            }
            DEBUG_PRINT("[EduCommunicationErrorThread] Leaving busy wait\n");
        }
    }
} eduCommunicationErrorThread;


auto ResumeEduCommunicationErrorThread() -> void
{
    eduCommunicationErrorThread.resume();
    DEBUG_PRINT("[EduCommunicationErrorThread] EduCommunicationErrorThread resumed\n");
}
}
