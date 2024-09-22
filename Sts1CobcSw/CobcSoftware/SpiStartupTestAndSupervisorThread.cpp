#include <Sts1CobcSw/CobcSoftware/FlashStartupTestThread.hpp>
#include <Sts1CobcSw/CobcSoftware/FramEpsStartupTestThread.hpp>
#include <Sts1CobcSw/CobcSoftware/RfStartupTestThread.hpp>
#include <Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramEpsSpi.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Utility/Debug.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
constexpr auto stackSize = 100U;
// TODO: Measure how long the startup tests really take to determine the correct timeout
constexpr auto startupTestTimeout = 100 * ms;
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
        static constexpr auto errorMessage = " failed to complete in time\n";
        static constexpr auto successMessage = " completed in time\n";

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
            if(CurrentRodosTime() > flash::spi.TransferEnd())
            {
                DEBUG_PRINT("Flash SPI timeout occurred\n");
                timeoutHappened = true;
                persistentVariables.template Increment<"nFlashErrors">();
            }
            if(CurrentRodosTime() > rf::spi.TransferEnd())
            {
                DEBUG_PRINT("RF SPI timeout occurred\n");
                timeoutHappened = true;
                persistentVariables.template Increment<"nRfErrors">();
            }
            if(timeoutHappened)
            {
                DEBUG_PRINT("Hardware reset and reboot");
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
