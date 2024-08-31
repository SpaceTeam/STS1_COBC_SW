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
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
constexpr auto stackSize = 100U;
// TODO: Measure how long the startup tests really take to determine the correct timeout
constexpr auto startupTestTimeout = 100 * RODOS::MILLISECONDS;
// TODO: Think about how often the supervision should run
constexpr auto supervisionPeriod = 1 * RODOS::SECONDS;


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
        using RODOS::AT;
        using RODOS::NOW;

        // TODO: Test if this works
        auto testWasSuccessful = ExecuteStartupTest(ResumeFramEpsStartupTestThread);
        if(not testWasSuccessful)
        {
            fram::framIsWorking.Store(false);
            persistentVariables.template Store<"epsIsWorking">(false);
        }
        testWasSuccessful = ExecuteStartupTest(ResumeFlashStartupTestThread);
        if(not testWasSuccessful)
        {
            persistentVariables.template Store<"flashIsWorking">(false);
            persistentVariables.template Store<"nFlashErrors">(
                persistentVariables.template Load<"nFlashErrors">() + 1);
        }
        testWasSuccessful = ExecuteStartupTest(ResumeRfStartupTestThread);
        if(not testWasSuccessful)
        {
            persistentVariables.template Store<"rfIsWorking">(false);
            persistentVariables.template Store<"nRfErrors">(
                persistentVariables.template Load<"nRfErrors">() + 1);
            AT(NOW() + 2 * RODOS::SECONDS);
            RODOS::hwResetAndReboot();
        }

        TIME_LOOP(0, supervisionPeriod)
        {
            auto timeoutHappened = false;
            if(NOW() > framEpsSpi.TransferEnd())
            {
                timeoutHappened = true;
            }
            if(NOW() > flash::spi.TransferEnd())
            {
                timeoutHappened = true;
                persistentVariables.template Store<"nFlashErrors">(
                    persistentVariables.template Load<"nFlashErrors">() + 1);
            }
            if(NOW() > rf::spi.TransferEnd())
            {
                timeoutHappened = true;
                persistentVariables.template Store<"nRfErrors">(
                    persistentVariables.template Load<"nRfErrors">() + 1);
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
    auto testEnd = RODOS::NOW() + startupTestTimeout;
    startupTestThreadResumeFuntion();
    RODOS::AT(testEnd);
    return RODOS::NOW() <= testEnd;
}
}
