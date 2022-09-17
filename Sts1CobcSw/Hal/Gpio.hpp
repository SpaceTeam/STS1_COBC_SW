#pragma once

#include <Sts1CobcSw/Hal/PinNames.hpp>
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
     * @brief Possible values of a single GPIO pin
     */
    enum class PinVal{
        zero,
        one
    };

    /**
     * @brief A wrapper for the init function of a GPIO pin. Only allows for single pins to be initialized
     * 
     * @param pin The pin to be initialized
     * @param pinType The pin type (input/output)
     * @param initVal The initial value for output pins (zero/one)
     * 
     * @returns Returns 0 on success, -1 on failure
     */
    auto InitPin(RODOS::GPIO_PIN pin, PinType pinType, PinVal initVal) -> int32_t;

    /**
     * @brief A wrapper for the setPins function of a GPIO pin.
     * 
     * @param pin The pin to be set
     * @param pinVal The value to be set (zero/one)
     */
    void SetPin(RODOS::GPIO_PIN pin, PinVal pinVal);
}