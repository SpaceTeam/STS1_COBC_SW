#include <Sts1CobcSw/Firmware/FramEpsStartupTestThread.hpp>

#include <Sts1CobcSw/Firmware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Sensors/Eps.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <rodos_no_using_namespace.h>

#include <array>


namespace sts1cobcsw
{
namespace
{
// Running the SpiSupervisor HW test showed that the minimum required stack size is ~800 bytes
constexpr auto stackSize = 850;


class FramEpsStartupTestThread : public RODOS::StaticThread<stackSize>
{
public:
    FramEpsStartupTestThread()
        : StaticThread("FramEpsStartupTestThread", framEpsStartupTestThreadPriority)
    {}


private:
    void init() override
    {}


    void run() override
    {
        SuspendUntil(endOfTime);
        DEBUG_PRINT("FRAM/EPS start-up test ...");
        fram::Initialize();
        auto deviceId = fram::ReadDeviceId();
        if(deviceId == fram::correctDeviceId)
        {
            fram::framIsWorking.Store(true);
        }
        else
        {
            DEBUG_PRINT(" failed to read correct FRAM device ID");
            fram::framIsWorking.Store(false);
        }
        persistentVariables.Store<"epsIsWorking">(true);
        eps::InitializeAdcs();
        auto adcData = eps::ReadAdcs();
        if(adcData == eps::AdcData{})
        {
            persistentVariables.Store<"epsIsWorking">(false);
        }
        else
        {
            persistentVariables.Store<"epsIsWorking">(true);
        }
        ResumeSpiStartupTestAndSupervisorThread();
        SuspendUntil(endOfTime);
    }
} framEpsStartupTestThread;
}


auto ResumeFramEpsStartupTestThread() -> void
{
    framEpsStartupTestThread.resume();
}
}
