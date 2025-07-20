#include <Sts1CobcSw/Firmware/EduCommunicationErrorThread.hpp>

#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
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
    void run() override
    {
        SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
        DEBUG_PRINT("Starting EDU communication error thread\n");
        while(true)
        {
            SuspendUntil(endOfTime);
            persistentVariables.Increment<"nEduCommunicationErrors">();
            DEBUG_PRINT("Resetting the EDU\n");
            edu::TurnOff();
            BusyWaitFor(eduShutDownDelay);
            // TODO: This thread must not turn on the EDU unconditionally. The power management
            // thread should take care of that probably because it already has the right conditions.
            edu::TurnOn();
            auto eduIsAlive = false;
            while(not eduIsAlive)
            {
                // TODO: Is this really necessary?
                yield();  // Force recalculation of scheduling!
                eduIsAliveBufferForCommunicationError.get(eduIsAlive);
            }
            DEBUG_PRINT_STACK_USAGE();
        }
    }
} eduCommunicationErrorThread;
}


auto ResumeEduCommunicationErrorThread() -> void
{
    eduCommunicationErrorThread.resume();
}
}
