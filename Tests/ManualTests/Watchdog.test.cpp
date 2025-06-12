#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <utility>


namespace sts1cobcsw
{
namespace
{
auto led1Gpio = hal::GpioPin(hal::led1Pin);


class WatchdogTest : public RODOS::StaticThread<>
{
    void init() override
    {
        led1Gpio.SetDirection(hal::PinDirection::out);
    }


    void run() override
    {
        led1Gpio.Reset();
        SuspendFor(800 * ms);
        led1Gpio.Set();
    }
} watchdogTest;
}
}
