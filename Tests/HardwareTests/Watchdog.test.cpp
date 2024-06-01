#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
static auto led1Gpio = hal::GpioPin(hal::led1Pin);
#if HW_VERSION >= 27
auto rfLatchupDisableGpioPin = hal::GpioPin(hal::rfLatchupDisablePin);
#endif


class WatchdogTest : public RODOS::StaticThread<>
{
    void init() override
    {
        led1Gpio.Direction(hal::PinDirection::out);
    }


    void run() override
    {
#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Reset();
#endif
        led1Gpio.Reset();
        RODOS::AT(RODOS::NOW() + 800 * RODOS::MILLISECONDS);
        led1Gpio.Set();
    }
} watchdogTest;
}
