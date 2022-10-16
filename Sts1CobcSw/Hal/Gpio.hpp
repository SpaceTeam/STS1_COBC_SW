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
    auto Set() -> void;
    auto Reset() -> void;
    auto Read() const -> PinState;

  private:
    mutable RODOS::HAL_GPIO pin_;
};


inline GpioPin::GpioPin(RODOS::GPIO_PIN pinIndex) : pin_(pinIndex)
{
}


inline auto GpioPin::Direction(PinDirection pinDirection) -> void
{
    pin_.reset();
    pin_.init(pinDirection == PinDirection::out, 1, 0);
}


inline auto GpioPin::Set() -> void
{
    pin_.setPins(1U);
}


inline auto GpioPin::Reset() -> void
{
    pin_.setPins(0U);
}


inline auto GpioPin::Read() const -> PinState
{
    return pin_.readPins() == 0U ? PinState::reset : PinState::set;
}
}