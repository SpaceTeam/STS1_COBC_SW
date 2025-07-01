#include <Sts1CobcSw/Firmware/SpiStartupTestAndSupervisorThread.hpp>

#include <Sts1CobcSw/Firmware/FlashStartupTestThread.hpp>
#include <Sts1CobcSw/Firmware/FramEpsStartupTestThread.hpp>
#include <Sts1CobcSw/Firmware/RfStartupTestThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Hal/Spis.hpp>
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
namespace
{
// Running the SpiSupervisor HW test in debug mode showed a max. stack usage of < 700 B. The golden
// test needs < 800 B.
constexpr auto stackSize = 1000 + EXTRA_SANITIZER_STACK_SIZE;

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
    {}


private:
    void init() override
    {}


    void run() override
    {
        // Briefly go to sleep to ensure that the low-priority startup test threads have started and
        // are waiting for the high-priority supervisor thread to resume them
        SuspendFor(initialSleepTime);

        // The messages are only used in DEBUG_PRINT() calls, so they are unused in release builds
        [[maybe_unused]] static constexpr auto errorMessage = " failed to complete in time\n";
        [[maybe_unused]] static constexpr auto successMessage = " completed in time\n";

        auto testWasSuccessful = ExecuteStartupTest(ResumeFramEpsStartupTestThread);
        DEBUG_PRINT(fram::framIsWorking.Load() ? " " : " and");
        if(not testWasSuccessful)
        {
            DEBUG_PRINT("%s", errorMessage);
            fram::framIsWorking.Store(false);
            persistentVariables.Store<"epsIsWorking">(false);
        }
        else
        {
            DEBUG_PRINT("%s", successMessage);
        }

        testWasSuccessful = ExecuteStartupTest(ResumeFlashStartupTestThread);
        DEBUG_PRINT(persistentVariables.Load<"flashIsWorking">() ? " " : " and");
        if(not testWasSuccessful)
        {
            DEBUG_PRINT("%s", errorMessage);
            persistentVariables.Store<"flashIsWorking">(false);
            persistentVariables.Increment<"nFlashErrors">();
        }
        else
        {
            DEBUG_PRINT("%s", successMessage);
        }

        testWasSuccessful = ExecuteStartupTest(ResumeRfStartupTestThread);
        DEBUG_PRINT(persistentVariables.Load<"rfIsWorking">() ? " " : " and");
        if(not testWasSuccessful)
        {
            DEBUG_PRINT("%s", errorMessage);
            persistentVariables.Store<"rfIsWorking">(false);
            persistentVariables.Increment<"nRfErrors">();
        }
        else
        {
            DEBUG_PRINT("%s", successMessage);
        }
        DEBUG_PRINT_STACK_USAGE();
        if(not persistentVariables.Load<"rfIsWorking">())
        {
            DEBUG_PRINT("Resetting and rebooting in 2 s\n");
            // TODO: Add a named constant for this delay
            SuspendFor(2 * s);
            RODOS::hwResetAndReboot();
        }

        auto i = 0U;
        TIME_LOOP(0, value_of(supervisionPeriod))
        {
            auto timeoutHappened = false;
            auto now = CurrentRodosTime();
            if(now > framEpsSpi.TransferEnd())
            {
                DEBUG_PRINT("FRAM/EPS SPI timeout occurred\n");
                timeoutHappened = true;
            }
            if(now > flashSpi.TransferEnd())
            {
                DEBUG_PRINT("Flash SPI timeout occurred\n");
                timeoutHappened = true;
                persistentVariables.Increment<"nFlashErrors">();
            }
            if(now > rfSpi.TransferEnd())
            {
                DEBUG_PRINT("RF SPI timeout occurred\n");
                timeoutHappened = true;
                persistentVariables.Increment<"nRfErrors">();
            }
            if(timeoutHappened)
            {
                RODOS::hwResetAndReboot();
            }
            if(i < 2)
            {
                DEBUG_PRINT_STACK_USAGE();
                ++i;
            }
        }
    }
} spiStartupTestAndSupervisorThread;
}


auto ResumeSpiStartupTestAndSupervisorThread() -> void
{
    spiStartupTestAndSupervisorThread.resume();
}


namespace
{
auto ExecuteStartupTest(void (*startupTestThreadResumeFuntion)()) -> bool
{
    auto testEnd = CurrentRodosTime() + startupTestTimeout;
    startupTestThreadResumeFuntion();
    SuspendUntil(testEnd);
    return CurrentRodosTime() <= testEnd;
}
}
}
