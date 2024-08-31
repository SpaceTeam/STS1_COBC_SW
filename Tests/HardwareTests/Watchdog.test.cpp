#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>

#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <strong_type/difference.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
static auto led1Gpio = hal::GpioPin(hal::led1Pin);


class WatchdogTest : public RODOS::StaticThread<>
{
    void init() override
    {
#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Direction(hal::PinDirection::out);
#endif
        led1Gpio.Direction(hal::PinDirection::out);
    }


    void run() override
    {
#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Reset();
#endif
        led1Gpio.Reset();
        SuspendFor(800 * ms);
        led1Gpio.Set();
    }
} watchdogTest;
}
