#include <Sts1CobcSw/CobcSoftware/FlashStartupTestThread.hpp>
#include <Sts1CobcSw/CobcSoftware/FramEpsStartupTestThread.hpp>
#include <Sts1CobcSw/CobcSoftware/RfStartupTestThread.hpp>
#include <Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/Spis.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <compare>


namespace sts1cobcsw
{
// Running the golden test for the supervisor thread showed that at least 850 bytes are needed
constexpr auto stackSize = 900 + EXTRA_SANITIZER_STACK_SIZE;

inline constexpr auto initialSleepTime = 10 * ms;
// TODO: Think about how often the supervision should run
constexpr auto supervisionPeriod = 1 * s;


auto ExecuteStartupTest(void (*startupTestThreadResumeFuntion)()) -> bool;


class SpiStartupTestAndSupervisorThread : public RODOS::StaticThread<stackSize>
{
public:
    SpiStartupTestAndSupervisorThread()
        : StaticThread("SpiStartupTestAndSupervisorThread",
                       spiStartupTestAndSupervisorThreadPriority)
    {
    }

private:
    void init() override
    {
    }


    void run() override
    {
        // Briefly go to sleep to ensure that the low-priority startup test threads have started and
        // are waiting for the high-priority supervisor thread to resume them
        SuspendFor(initialSleepTime);

        // The messages are only used in DEBUG_PRINT() calls, so they are unused in release builds
        [[maybe_unused]] static constexpr auto errorMessage = " failed to complete in time\n";
        [[maybe_unused]] static constexpr auto successMessage = " completed in time\n";

        auto testWasSuccessful = ExecuteStartupTest(ResumeFramEpsStartupTestThread);
        DEBUG_PRINT(fram::framIsWorking.Load() ? "\n" : " and");
        if(not testWasSuccessful)
        {
            DEBUG_PRINT("%s", errorMessage);
            fram::framIsWorking.Store(false);
            persistentVariables.template Store<"epsIsWorking">(false);
        }
        else
        {
            DEBUG_PRINT("%s", successMessage);
        }

        testWasSuccessful = ExecuteStartupTest(ResumeFlashStartupTestThread);
        DEBUG_PRINT(persistentVariables.template Load<"flashIsWorking">() ? "\n" : " and");
        if(not testWasSuccessful)
        {
            DEBUG_PRINT("%s", errorMessage);
            persistentVariables.template Store<"flashIsWorking">(false);
            persistentVariables.template Increment<"nFlashErrors">();
        }
        else
        {
            DEBUG_PRINT("%s", successMessage);
        }

        testWasSuccessful = ExecuteStartupTest(ResumeRfStartupTestThread);
        DEBUG_PRINT(persistentVariables.template Load<"rfIsWorking">() ? "\n" : " and");
        if(not testWasSuccessful)
        {
            DEBUG_PRINT("%s", errorMessage);
            persistentVariables.template Store<"rfIsWorking">(false);
            persistentVariables.template Increment<"nRfErrors">();
            DEBUG_PRINT("Resetting and rebooting in 2 s\n");
            // TODO: Add a named constant for this delay
            SuspendFor(2 * s);
            RODOS::hwResetAndReboot();
        }
        else
        {
            DEBUG_PRINT("%s", successMessage);
        }

        TIME_LOOP(0, value_of(supervisionPeriod))
        {
            auto timeoutHappened = false;
            if(CurrentRodosTime() > framEpsSpi.TransferEnd())
            {
                DEBUG_PRINT("FRAM/EPS SPI timeout occurred\n");
                timeoutHappened = true;
            }
            if(CurrentRodosTime() > flashSpi.TransferEnd())
            {
                DEBUG_PRINT("Flash SPI timeout occurred\n");
                timeoutHappened = true;
                persistentVariables.template Increment<"nFlashErrors">();
            }
            if(CurrentRodosTime() > rfSpi.TransferEnd())
            {
                DEBUG_PRINT("RF SPI timeout occurred\n");
                timeoutHappened = true;
                persistentVariables.template Increment<"nRfErrors">();
            }
            if(timeoutHappened)
            {
                RODOS::hwResetAndReboot();
            }
        }
    }
} spiStartupTestAndSupervisorThread;


auto ResumeSpiStartupTestAndSupervisorThread() -> void
{
    spiStartupTestAndSupervisorThread.resume();
}


auto ExecuteStartupTest(void (*startupTestThreadResumeFuntion)()) -> bool
{
    auto testEnd = CurrentRodosTime() + startupTestTimeout;
    startupTestThreadResumeFuntion();
    SuspendUntil(testEnd);
    return CurrentRodosTime() <= testEnd;
}
}
