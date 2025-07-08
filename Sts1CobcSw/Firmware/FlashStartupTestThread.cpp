#include <Sts1CobcSw/Firmware/FlashStartupTestThread.hpp>

#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Flash/Flash.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
namespace
{
// Running the SpiSupervisor HW test in debug mode showed a max. stack usage of < 600 B
constexpr auto stackSize = 800;


class FlashStartupTestThread : public RODOS::StaticThread<stackSize>
{
public:
    FlashStartupTestThread()
        : StaticThread("FlashStartupTestThread", flashStartupTestThreadPriority)
    {}


private:
    void init() override
    {}


    void run() override
    {
        SuspendUntil(endOfTime);
        DEBUG_PRINT("Flash start-up test ...\n");
        flash::Initialize();
        auto jedecId = flash::ReadJedecId();
        if(jedecId.deviceId == flash::correctJedecId.deviceId
           && jedecId.manufacturerId == flash::correctJedecId.manufacturerId)
        {
            persistentVariables.Store<"flashIsWorking">(true);
        }
        else
        {
            DEBUG_PRINT("  failed to read correct flash JEDEC ID\n");
            persistentVariables.Store<"flashIsWorking">(false);
        }
        DEBUG_PRINT_STACK_USAGE();
        ResumeStartupAndSpiSupervisorThread();
        SuspendUntil(endOfTime);
    }
} flashStartupTestThread;
}


auto ResumeFlashStartupTestThread() -> void
{
    flashStartupTestThread.resume();
}
}
