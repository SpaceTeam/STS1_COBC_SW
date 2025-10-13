#include <Sts1CobcSw/WatchdogTimers/WatchdogTimers.hpp>

#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <utility>


namespace sts1cobcsw
{
namespace
{
auto watchdogClearGpioPin = hal::GpioPin(hal::watchdogClearPin);
auto resetdogClearGpioPin1 = hal::GpioPin(hal::resetdogClearPin1);
auto resetdogClearGpioPin2 = hal::GpioPin(hal::resetdogClearPin2);
}


namespace wdt
{
auto Initialize() -> void
{
    watchdogClearGpioPin.SetDirection(hal::PinDirection::out);
    watchdogClearGpioPin.Reset();
}


auto Feed() -> void
{
    // FIXME: const value correct?
    static constexpr auto watchdogFeedPulseDuration = 2 * ms;
    watchdogClearGpioPin.Set();
    SuspendFor(watchdogFeedPulseDuration);
    watchdogClearGpioPin.Reset();
}
}


namespace rdt
{
auto Initialize() -> void
{
    resetdogClearGpioPin1.SetDirection(hal::PinDirection::out);
    resetdogClearGpioPin1.Reset();
    resetdogClearGpioPin2.SetDirection(hal::PinDirection::out);
    resetdogClearGpioPin2.Reset();
}


auto Feed() -> void
{
    // FIXME: const value correct?
    static constexpr auto resetdogFeedPulseDuration = 2 * ms;
    resetdogClearGpioPin1.Set();
    resetdogClearGpioPin2.Set();
    SuspendFor(resetdogFeedPulseDuration);
    resetdogClearGpioPin1.Reset();
    resetdogClearGpioPin2.Reset();
}
}
}
