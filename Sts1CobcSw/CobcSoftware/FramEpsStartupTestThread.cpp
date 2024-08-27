#include <Sts1CobcSw/CobcSoftware/FramEpsStartupTestThread.hpp>
#include <Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Periphery/Eps.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>

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
        RODOS::AT(RODOS::END_OF_TIME);
        fram::Initialize();
        auto deviceId = fram::ReadDeviceId();
        if(deviceId != fram::correctDeviceId)
        {
            fram::framIsWorking.Store(false);
        }
        eps::Initialize();
        (void)eps::Read();
        ResumeSpiStartupTestAndSupervisorThread();
        RODOS::AT(RODOS::END_OF_TIME);
    }
} framEpsStartupTestThread;


auto ResumeFramEpsStartupTestThread() -> void
{
    framEpsStartupTestThread.resume();
}
}
