#include <Sts1CobcSw/FlashStartupTestThread.hpp>
#include <Sts1CobcSw/FramEpsStartupTestThread.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Periphery/FramEpsSpi.hpp>
#include <Sts1CobcSw/Periphery/PersistentState.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/RfStartupTestThread.hpp>
#include <Sts1CobcSw/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/ThreadPriorities.hpp>
#include <Sts1CobcSw/Utility/Debug.hpp>

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
            DEBUG_PRINT("FramEpsStartupTest was not completed in time");
            persistentstate::FramIsWorking(false);
            persistentstate::EpsIsWorking(false);
        }
        else
        {
            DEBUG_PRINT("FramEpsStartupTest completed in time");
        }
        testWasSuccessful = ExecuteStartupTest(ResumeFlashStartupTestThread);
        if(not testWasSuccessful)
        {
            DEBUG_PRINT("FlashStartupTest was not completed in time");
            persistentstate::FlashIsWorking(false);
            persistentstate::FlashErrorCounter(persistentstate::FlashErrorCounter() + 1);
        }
        else
        {
            DEBUG_PRINT("FlashStartupTest completed in time");
        }
        testWasSuccessful = ExecuteStartupTest(ResumeRfStartupTestThread);
        if(not testWasSuccessful)
        {
            DEBUG_PRINT("RfStartupTest was not completed in time");
            persistentstate::RfIsWorking(false);
            persistentstate::RfErrorCounter(persistentstate::RfErrorCounter() + 1);
            AT(NOW() + 2 * RODOS::SECONDS);
            RODOS::hwResetAndReboot();
        }
        else
        {
            DEBUG_PRINT("RfStartupTest completed in time");
        }

        TIME_LOOP(0, supervisionPeriod)
        {
            auto timeoutHappened = false;
            if(NOW() > framEpsSpi.TransferEnd())
            {
                DEBUG_PRINT("FramEps timeout occurred");
                timeoutHappened = true;
            }
            if(NOW() > flash::spi.TransferEnd())
            {
                DEBUG_PRINT("Flash timeout occurred");
                timeoutHappened = true;
                persistentstate::FlashErrorCounter(persistentstate::FlashErrorCounter() + 1);
            }
            if(NOW() > rf::spi.TransferEnd())
            {
                DEBUG_PRINT("Rf timeout occurred");
                timeoutHappened = true;
                persistentstate::RfErrorCounter(persistentstate::RfErrorCounter() + 1);
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
    auto testEnd = RODOS::NOW() + startupTestTimeout;
    startupTestThreadResumeFuntion();
    RODOS::AT(testEnd);
    return RODOS::NOW() <= testEnd;
}
}
