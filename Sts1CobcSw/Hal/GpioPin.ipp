#pragma once


#include <Sts1CobcSw/Hal/GpioPin.hpp>


namespace sts1cobcsw::hal
{
inline GpioPin::GpioPin(RODOS::GPIO_PIN pinIndex) : pin_(pinIndex)
{
}


inline auto GpioPin::Direction(PinDirection pinDirection) -> void
{
    pin_.reset();
    pin_.init(pinDirection == PinDirection::out, 1, 0);
}


inline auto GpioPin::OutputType(PinOutputType pinOutputType) -> void
{
    pin_.config(RODOS::GPIO_CFG_OPENDRAIN_ENABLE,
                pinOutputType == PinOutputType::openDrain ? 1U : 0U);
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
    return pin_.readPins() == 0 ? PinState::reset : PinState::set;
}
}
