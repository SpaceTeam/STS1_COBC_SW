#include <Sts1CobcSw/CobcSoftware/FlashStartupTestThread.hpp>
#include <Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Utility/Debug.hpp>
#include <Sts1CobcSw/Utility/RodosTime.hpp>

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
        DEBUG_PRINT("Flash start-up test ...");
        SuspendUntil(endOfTime);
        flash::Initialize();
        auto jedecId = flash::ReadJedecId();
        if(jedecId.deviceId != flash::correctJedecId.deviceId
           || jedecId.manufacturerId != flash::correctJedecId.manufacturerId)
        {
            DEBUG_PRINT(" failed to read correct flash JEDEC ID");
            persistentVariables.template Store<"flashIsWorking">(false);
        }
        ResumeSpiStartupTestAndSupervisorThread();
        SuspendUntil(endOfTime);
    }
} flashStartupTestThread;


auto ResumeFlashStartupTestThread() -> void
{
    flashStartupTestThread.resume();
}
}
