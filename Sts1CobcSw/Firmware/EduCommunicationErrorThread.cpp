#include <Sts1CobcSw/Firmware/EduCommunicationErrorThread.hpp>

#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Firmware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <utility>


namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 2000U;
constexpr auto eduShutDownDelay = 2 * s;


class EduCommunicationErrorThread : public RODOS::StaticThread<stackSize>
{
public:
    EduCommunicationErrorThread()
        : StaticThread("EduCommunicationThread", eduCommunicationErrorThreadPriority)
    {}


private:
    void init() override
    {}


    void run() override
    {
        while(true)
        {
            SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
            SuspendUntil(endOfTime);

            persistentVariables.Increment<"nEduCommunicationErrors">();

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
}


auto ResumeEduCommunicationErrorThread() -> void
{
    eduCommunicationErrorThread.resume();
    DEBUG_PRINT("[EduCommunicationErrorThread] EduCommunicationErrorThread resumed\n");
}
}
