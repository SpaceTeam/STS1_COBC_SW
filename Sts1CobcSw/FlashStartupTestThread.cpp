#include <Sts1CobcSw/FlashStartupTestThread.hpp>
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Periphery/PersistentState.hpp>
#include <Sts1CobcSw/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/ThreadPriorities.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
constexpr auto stackSize = 100U;


class FlashStartupTestThread : public RODOS::StaticThread<stackSize>
{
public:
    FlashStartupTestThread()
        : StaticThread("FlashStartupTestThread", flashStartupTestThreadPriority)
    {
    }


private:
    void init() override
    {
    }


    void run() override
    {
        RODOS::AT(RODOS::END_OF_TIME);
        flash::Initialize();
        auto jedecId = flash::ReadJedecId();
        if(jedecId.deviceId != flash::correctJedecId.deviceId
           || jedecId.manufacturerId != flash::correctJedecId.manufacturerId)
        {
            persistentstate::FlashIsWorking(/*value=*/false);
        }
        ResumeSpiStartupTestAndSupervisorThread();
        RODOS::AT(RODOS::END_OF_TIME);
    }
} flashStartupTestThread;


auto ResumeFlashStartupTestThread() -> void
{
    flashStartupTestThread.resume();
}
}
