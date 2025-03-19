#include <Sts1CobcSw/CobcSoftware/FlashStartupTestThread.hpp>
#include <Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Utility/RodosTime.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
// Running the SpiSupervisor HW test showed that the minimum required stack size is ~560 bytes
constexpr auto stackSize = 600;


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
        SuspendUntil(endOfTime);
        DEBUG_PRINT("Flash start-up test ...");
        flash::Initialize();
        auto jedecId = flash::ReadJedecId();
        if(jedecId.deviceId == flash::correctJedecId.deviceId
           && jedecId.manufacturerId == flash::correctJedecId.manufacturerId)
        {
            persistentVariables.template Store<"flashIsWorking">(true);
        }
        else
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
