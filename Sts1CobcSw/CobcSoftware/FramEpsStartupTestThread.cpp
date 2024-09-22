#include <Sts1CobcSw/CobcSoftware/FramEpsStartupTestThread.hpp>
#include <Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Periphery/Eps.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Utility/Debug.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <rodos_no_using_namespace.h>

#include <array>


namespace sts1cobcsw
{
constexpr auto stackSize = 100U;


class FramEpsStartupTestThread : public RODOS::StaticThread<stackSize>
{
public:
    FramEpsStartupTestThread()
        : StaticThread("FramEpsStartupTestThread", framEpsStartupTestThreadPriority)
    {
    }


private:
    void init() override
    {
    }


    void run() override
    {
        DEBUG_PRINT("FRAM/EPS start-up test ...");
        SuspendUntil(endOfTime);
        fram::Initialize();
        auto deviceId = fram::ReadDeviceId();
        if(deviceId != fram::correctDeviceId)
        {
            DEBUG_PRINT(" failed to read correct FRAM device ID");
            fram::framIsWorking.Store(false);
        }
        eps::Initialize();
        (void)eps::Read();
        ResumeSpiStartupTestAndSupervisorThread();
        SuspendUntil(endOfTime);
    }
} framEpsStartupTestThread;


auto ResumeFramEpsStartupTestThread() -> void
{
    framEpsStartupTestThread.resume();
}
}
