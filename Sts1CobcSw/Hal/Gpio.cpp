#include <Sts1CobcSw/Hal/Gpio.hpp>

#include <rodos.h>

namespace sts1cobcsw::hal
{
auto InitPin(RODOS::HAL_GPIO & pin, PinType pinType, PinVal initVal) -> int32_t
{
    bool isOutput = (pinType == PinType::output);
    uint32_t initValUint = (initVal == PinVal::zero ? 0 : 1);
    return pin.init(isOutput, 1, initValUint);
}

void SetPin(RODOS::HAL_GPIO & pin, PinVal pinVal)
{
    uint32_t pinValUint = (pinVal == PinVal::zero ? 0 : 1);
    pin.setPins(pinValUint);
}
}