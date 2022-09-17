#include <Sts1CobcSw/Hal/Gpio.hpp>
#include <rodos.h>

namespace sts1cobcsw::hal{
    auto InitPin(RODOS::GPIO_PIN pin, PinType pinType, PinVal initVal) -> int32_t{
        RODOS::HAL_GPIO halGpio(pin);
        bool isOutput = (pinType == PinType::output);
        uint32_t initValUint = (initVal == PinVal::zero ? 0 : 1);
        return halGpio.init(isOutput, 1, initValUint);
    }

    void SetPin(RODOS::GPIO_PIN pin, PinVal pinVal){
        RODOS::HAL_GPIO halGpio(pin);
        uint32_t pinValUint = (pinVal == PinVal::zero ? 0 : 1);
        halGpio.setPins(pinValUint);
    }
}