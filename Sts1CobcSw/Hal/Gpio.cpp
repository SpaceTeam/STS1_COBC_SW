#include <Sts1CobcSw/Hal/Gpio.hpp>

#include <rodos.h>

namespace sts1cobcsw::hal
{
auto InitPin(RODOS::HAL_GPIO & pin, PinType pinType, type_safe::bool_t initVal) -> int32_t
{
    bool isOutput = (pinType == PinType::output);
    uint32_t initValUint = (initVal ? 1U : 0U);
    return pin.init(isOutput, 1, initValUint);
}

void SetPin(RODOS::HAL_GPIO & pin, type_safe::bool_t pinVal)
{
    uint32_t pinValUint = (pinVal ? 1U : 0U);
    pin.setPins(pinValUint);
}


auto ReadPin(RODOS::HAL_GPIO & pin) -> type_safe::bool_t
{
    auto pinVal = pin.readPins();
    type_safe::bool_t boolVal = (pinVal != 0U);
    return boolVal;
}
}