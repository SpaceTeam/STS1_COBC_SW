#include <Sts1CobcSw/Hal/Gpio.hpp>
#include <rodos.h>

namespace sts1cobcsw::hal{
    auto InitGpioPin(RODOS::HAL_GPIO &pin, PinType pinType, InitVal initVal){
        bool isOutput = (pinType == PinType::output);
        uint32_t initValUint = (initVal == InitVal::zeroInit ? 0 : 1);
        return pin.init(isOutput, 1, initValUint);
    }
}