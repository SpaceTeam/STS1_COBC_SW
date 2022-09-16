#pragma once

#include <cstdint>
#include <rodos.h>

namespace sts1cobcsw::hal
{
    /**
     * @brief The pin types of a GPIO pin
     */
    enum class PinType{
        input,
        output
    };

    /**
     * @brief Possible initial values of a single GPIO pin
     */
    enum class InitVal{
        zeroInit,
        oneInit
    };

    /**
     * @brief A wrapper for the init function of a GPIO pin
     * 
     * @param pinType The pin type (Input/Output)
     * @param initVal The initial value for output pins (ZeroInit/OneInit)
     * 
     * @returns Returns 0 on success, -1 on failure
     */
    auto InitGpioPin(RODOS::HAL_GPIO &pin, PinType pinType, InitVal initVal);
}