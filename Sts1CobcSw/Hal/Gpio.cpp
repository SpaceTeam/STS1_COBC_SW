#include <Sts1CobcSw/Hal/Gpio.hpp>


namespace sts1cobcsw::hal
{
void SetPinDirection(RODOS::HAL_GPIO * pin, PinDirection direction)
{
    auto isOutput = (direction == PinDirection::out);
    pin->reset();
    pin->init(isOutput, 1);
}


void SetPin(RODOS::HAL_GPIO & pin, PinState state)
{
    auto pinValue = (state == PinState::set ? 1U : 0U);
    pin.setPins(pinValue);
}


auto ReadPin(RODOS::HAL_GPIO & pin)
{
    auto pinValue = pin.readPins();
    auto pinState = (pinValue == 0U) ? PinState::reset : PinState::set;
    return pinState;
}
}