#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/ThreadPriorities.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
static auto led2Gpio = hal::GpioPin(hal::led2Pin);
static auto watchdogClearGpio = hal::GpioPin(hal::watchdogClearPin);


class WatchdogClearTest : public RODOS::StaticThread<>
{
public:
    WatchdogClearTest() : StaticThread("WatchdogClearTest", watchdogClearThreadPriority)
    {
    }


private:
    void init() override
    {
        led2Gpio.Direction(hal::PinDirection::out);
        watchdogClearGpio.Direction(hal::PinDirection::out);
    }


    void run() override
    {
        auto toggle = true;
        TIME_LOOP(0, 800 * RODOS::MILLISECONDS)
        {
            if(toggle)
            {
                led2Gpio.Reset();
                watchdogClearGpio.Reset();
            }
            else
            {
                led2Gpio.Set();
                watchdogClearGpio.Set();
            }
            toggle = not toggle;
        }
    }
} watchdogClearTest;
}
