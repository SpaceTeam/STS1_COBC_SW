#include <Tests/HardwareSetup/RfLatchupProtection.hpp>

#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>

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
        watchdogClearGpio.SetDirection(hal::PinDirection::out);
#endif
    }


    auto run() -> void override
    {
        EnableRfLatchupProtection();
#ifdef USE_WATCHDOG
        auto toggle = true;
        TIME_LOOP(0, 800 * RODOS::MILLISECONDS)
        {
            if(toggle)
            {
                watchdogClearGpio.Reset();
            }
            else
            {
                watchdogClearGpio.Set();
            }
            toggle = not toggle;
        }
#endif
    }
} hardwareSetupThread;
}
}
