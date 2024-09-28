#pragma once


#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw::hal
{
enum class PinDirection
{
    in,
    out
};


enum class PinOutputType
{
    pushPull,
    openDrain
};


enum class PinState
{
    set,
    reset
};


class GpioPin
{
public:
    // Implicit conversion from GPIO_PIN is very convenient (see Gpio.test.cpp)
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    GpioPin(RODOS::GPIO_PIN pinIndex);

    auto Direction(PinDirection pinDirection) -> void;
    auto OutputType(PinOutputType pinOutputType) -> void;
    auto Set() -> void;
    auto Reset() -> void;
    [[nodiscard]] auto Read() const -> PinState;

private:
    mutable RODOS::HAL_GPIO pin_;
};
}


#include <Sts1CobcSw/Hal/GpioPin.ipp>
