#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
auto ledGpio = hal::GpioPin(hal::led1Pin);


class WatchdogTest : public RODOS::StaticThread<>
{
    void init() override
    {
        ledGpio.Direction(hal::PinDirection::out);
    }


    void run() override
    {
        ledGpio.Reset();
        RODOS::AT(RODOS::NOW() + 800 * RODOS::MILLISECONDS);
        ledGpio.Set();
    }
} watchdogTest;
}
