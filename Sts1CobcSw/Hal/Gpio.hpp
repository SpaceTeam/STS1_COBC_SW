#pragma once

#include <Sts1CobcSw/Hal/PinNames.hpp>
#include <type_safe/types.hpp>

#include <rodos.h>

#include <cstdint>

namespace sts1cobcsw::hal
{
/**
 * @brief The pin types of a GPIO pin
 */
enum class PinType
{
    input,
    output
};

/**
 * @brief A wrapper for the init function of a GPIO pin. Only allows for single pins to be
 * initialized
 *
 * @param pin The pin to be initialized
 * @param pinType The pin type (input/output)
 * @param initVal The initial value for output pins
 *
 * @returns Returns 0 on success, -1 on failure
 */
auto InitPin(RODOS::HAL_GPIO & pin, PinType pinType, type_safe::bool_t initVal) -> int32_t;

/**
 * @brief A wrapper for the setPins function of a GPIO pin.
 *
 * @param pin The pin to be set
 * @param pinVal The value to be set
 */
void SetPin(RODOS::HAL_GPIO & pin, type_safe::bool_t pinVal);
}