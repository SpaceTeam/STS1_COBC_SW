#include <Sts1CobcSw/Periphery/Eps.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/ThreadPriorities.hpp>


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
        // Suspend until EOT
        RODOS::AT(RODOS::END_OF_TIME);

        // Initialize devices and read its ID
        fram::Initialize();
        auto deviceId = fram::ReadDeviceId();
        if(deviceId != fram::correctDeviceId)
        {
            fram::framIsWorking = false;
        }

        eps::Initialize();
        eps::Read()

            // Wake up SPI startup test and supervisor thread
            ResumeSpiStartupTestAndSupervisorThread();

        // Suspend until EOT
        RODOS::AT(RODOS::END_OF_TIME);
    }
} framEpsStartupTestThread;
}
