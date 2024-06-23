#include <Sts1CobcSw/FlashStartupTestThread.hpp>
#include <Sts1CobcSw/FramEpsStartupTestThread.hpp>
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramEpsSpi.hpp>
#include <Sts1CobcSw/Periphery/PersistentState.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/RfStartupTestThread.hpp>
#include <Sts1CobcSw/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/ThreadPriorities.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
constexpr auto stackSize = 100U;


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
        // Wake up startup test threads
        ResumeFramEpsStartupTestThread();
        ResumeFlashStartupTestThread();
        ResumeRfStartupTestThread();
        while(RODOS::NOW() <= framEpsSpi.TransferEnd() && RODOS::NOW() <= flash::spi.TransferEnd()
              && RODOS::NOW() <= rf::spi.TransferEnd())
        {
        }

        // Is FRAM/EPS ok?
        if(RODOS::NOW() > framEpsSpi.TransferEnd())
        {
            persistentstate::FramIsWorking(false);
        }

        // Is FLASH ok?
        if(RODOS::NOW() > flash::spi.TransferEnd())
        {
            if(persistentstate::FramIsWorking())
            {
                persistentstate::FlashErrorCounter(persistentstate::FlashErrorCounter() + 1);
            }
            persistentstate::FlashIsWorking(false);
        }

        // Is RF working?
        if(RODOS::NOW() > rf::spi.TransferEnd())
        {
            if(persistentstate::FramIsWorking())
            {
                persistentstate::RfErrorCounter(persistentstate::RfErrorCounter() + 1);
            }
            RODOS::AT(RODOS::NOW() + 2 * RODOS::SECONDS);
            RODOS::hwResetAndReboot();
        }

        // Watch over SPI
        while(RODOS::NOW() <= framEpsSpi.TransferEnd() && RODOS::NOW() <= flash::spi.TransferEnd()
              && RODOS::NOW() <= rf::spi.TransferEnd())
        {
        }
        if(RODOS::NOW() > framEpsSpi.TransferEnd())
        {
            RODOS::hwResetAndReboot();
        }
        else if(RODOS::NOW() > flash::spi.TransferEnd())
        {
            persistentstate::FlashErrorCounter(persistentstate::FlashErrorCounter() + 1);
            RODOS::hwResetAndReboot();
        }
        else if(RODOS::NOW() > rf::spi.TransferEnd())
        {
            persistentstate::RfErrorCounter(persistentstate::RfErrorCounter() + 1);
            RODOS::hwResetAndReboot();
        }
        RODOS::hwResetAndReboot();
    }
} spiStartupTestAndSupervisorThread;


auto ResumeSpiStartupTestAndSupervisorThread() -> void
{
    spiStartupTestAndSupervisorThread.resume();
}
}
