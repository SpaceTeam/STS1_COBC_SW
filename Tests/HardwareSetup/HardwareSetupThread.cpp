#include <Tests/HardwareSetup/RfLatchupProtection.hpp>

#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#ifdef USE_WATCHDOG
    #include <Sts1CobcSw/WatchdogTimers/WatchdogTimers.hpp>
#endif

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
namespace
{
auto watchdogClearGpio = hal::GpioPin(hal::watchdogClearPin);


class HardwareSetupThread : public RODOS::StaticThread<>
{
public:
    HardwareSetupThread() : StaticThread("HardwareSetupThread", MAX_THREAD_PRIORITY)
    {}


private:
    auto init() -> void override
    {
        InitializeRfLatchupDisablePins();
#ifdef USE_WATCHDOG
        wdt::Initialize();
#endif
    }


    auto run() -> void override
    {
        EnableRfLatchupProtection();
#ifdef USE_WATCHDOG
        TIME_LOOP(0, 800 * RODOS::MILLISECONDS)
        {
            wdt::Feed();
        }
#endif
    }
} hardwareSetupThread;
}
}
