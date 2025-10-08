#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/Firmware/EduPowerManagementThread.hpp>
#include <Sts1CobcSw/Firmware/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
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
constexpr auto stackSize = 5000U;
constexpr auto eduIsAliveCheckInterval = 1 * s;


auto SuspendUntilEduIsAliveAndHasUpdate() -> void;
auto ProcessEduUpdate() -> Result<void>;


class EduListenerThread : public RODOS::StaticThread<stackSize>
{
public:
    EduListenerThread() : StaticThread("EduListenerThread", eduListenerThreadPriority)
    {}


private:
    void init() override
    {
        edu::updateGpioPin.SetDirection(hal::PinDirection::in);
        edu::updateGpioPin.SetInterruptSensitivity(hal::InterruptSensitivity::risingEdge);
        edu::dosiEnableGpioPin.SetDirection(hal::PinDirection::out);
    }


    void run() override
    {
        SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
        DEBUG_PRINT("Starting EDU listener thread\n");
        while(true)
        {
            SuspendUntilEduIsAliveAndHasUpdate();
            auto result = ProcessEduUpdate();
            DEBUG_PRINT_STACK_USAGE();
            if(result.has_error())
            {
                persistentVariables.Increment<"nEduCommunicationErrors">();
                ResetEdu();
            }
        }
    }
} eduListenerThread;


auto SuspendUntilEduIsAliveAndHasUpdate() -> void
{
    while(true)
    {
        auto eduIsAlive = false;
        eduIsAliveBufferForListener.get(eduIsAlive);
        if(eduIsAlive and edu::updateGpioPin.Read() == hal::PinState::set)
        {
            return;
        }
        SuspendFor(eduIsAliveCheckInterval);
    }
}


auto ProcessEduUpdate() -> Result<void>
{
    auto getStatusResult = edu::GetStatus();
    if(getStatusResult.has_error())
    {
        DEBUG_PRINT("Failed to get EDU status: %s\n", ToCZString(getStatusResult.error()));
        return getStatusResult.error();
    }
    auto const & status = getStatusResult.value();
    switch(status.statusType)
    {
        case edu::StatusType::programFinished:
        {
            DEBUG_PRINT("EDU program %i finished with exit code %i\n",
                        value_of(status.programId),
                        status.exitCode);
            ResumeEduProgramQueueThread();
            break;
        }
        case edu::StatusType::resultsReady:
        {
            // ReturnResult is sent right after GetStatus, so we assume that the EDU is still alive
            // here.
            auto returnResultResult =
                edu::ReturnResult({.programId = status.programId, .startTime = status.startTime});
            if(returnResultResult.has_error())
            {
                DEBUG_PRINT("Failed to get EDU result: %s\n",
                            ToCZString(returnResultResult.error()));
                return returnResultResult.error();
            }
            DEBUG_PRINT("Received and stored EDU result for program %i with start time %u\n",
                        value_of(status.programId),
                        static_cast<unsigned>(value_of(status.startTime)));
            persistentVariables.Store<"newEduResultIsAvailable">(true);
            break;
        }
        case edu::StatusType::enableDosimeter:
            DEBUG_PRINT("Enabling dosimeter\n");
            edu::dosiEnableGpioPin.Set();
            break;
        case edu::StatusType::disableDosimeter:
            DEBUG_PRINT("Disabling dosimeter\n");
            edu::dosiEnableGpioPin.Reset();
            break;
        case edu::StatusType::noEvent:  // NOLINT(bugprone-branch-clone)
            DEBUG_PRINT("No EDU event\n");
            break;
        case edu::StatusType::invalid:
            DEBUG_PRINT("Received invalid EDU status\n");
            break;
    }
    return outcome_v2::success();
}
}
}
