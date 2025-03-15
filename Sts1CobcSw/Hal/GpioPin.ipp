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


inline auto GpioPin::EnableInterrupts() -> void
{
    pin_.interruptEnable(true);
}


inline auto GpioPin::DisableInterrupts() -> void
{
    pin_.interruptEnable(false);
}


inline auto GpioPin::ResetInterruptStatus() -> void
{
    pin_.resetInterruptEventStatus();
}


inline auto GpioPin::Read() const -> PinState
{
    return pin_.readPins() == 0 ? PinState::reset : PinState::set;
}


inline auto GpioPin::InterruptOccurred() const -> bool
{
    return pin_.isDataReady();
}


inline auto GpioPin::GpioEventReceiver::SetInterruptHandler(void (*handler)()) -> void
{
    interruptHandler_ = handler;
}
}
