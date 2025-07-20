#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>
#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/Firmware/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/Firmware/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
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
constexpr auto eduIsAliveCheckInterval = 1 * s;


auto SuspendUntilEduIsAliveAndHasUpdate() -> void;


class EduListenerThread : public RODOS::StaticThread<>
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
            auto getStatusResult = edu::GetStatus();
            if(getStatusResult.has_error())
            {
                DEBUG_PRINT("Failed to get EDU status: %s\n", ToCZString(getStatusResult.error()));
                ResumeEduCommunicationErrorThread();
                continue;
            }
            auto const & status = getStatusResult.value();
            // TODO: Extract the if-else chain into a separate function and maybe use a switch
            if(status.statusType == edu::StatusType::programFinished)
            {
                auto programStatus = status.exitCode == 0
                                       ? edu::ProgramStatus::programExecutionSucceeded
                                       : edu::ProgramStatus::programExecutionFailed;
                DEBUG_PRINT("EDU program %i finished with exit code %i\n",
                            value_of(status.programId),
                            status.exitCode);
                edu::UpdateProgramStatusHistory(status.programId, status.startTime, programStatus);
                ResumeEduProgramQueueThread();
            }
            else if(status.statusType == edu::StatusType::resultsReady)
            {
                // ReturnResult is sent right after GetStatus, so we assume that the EDU is still
                // alive here.
                auto returnResultResult = edu::ReturnResult(
                    {.programId = status.programId, .startTime = status.startTime});
                if(returnResultResult.has_error())
                {
                    DEBUG_PRINT("Failed to get EDU result: %s\n",
                                ToCZString(returnResultResult.error()));
                    ResumeEduCommunicationErrorThread();
                    continue;
                }
                DEBUG_PRINT("Received and stored EDU result for program %i with start time %u\n",
                            value_of(status.programId),
                            static_cast<unsigned>(value_of(status.startTime)));
                edu::UpdateProgramStatusHistory(
                    status.programId, status.startTime, edu::ProgramStatus::resultFileTransfered);
            }
            else if(status.statusType == edu::StatusType::enableDosimeter)
            {
                DEBUG_PRINT("Enabling dosimeter\n");
                edu::dosiEnableGpioPin.Set();
            }
            else if(status.statusType == edu::StatusType::disableDosimeter)
            {
                DEBUG_PRINT("Disabling dosimeter\n");
                edu::dosiEnableGpioPin.Reset();
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
}
}
