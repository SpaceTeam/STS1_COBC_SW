#pragma once

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw::hal
{
enum class PinDirection
{
    in,
    out
};


enum class PinState
{
    set,
    reset
};


void SetPinDirection(RODOS::HAL_GPIO * pin, PinDirection direction);


void SetPin(RODOS::HAL_GPIO * pin, PinState state);


auto ReadPin(RODOS::HAL_GPIO const & pin) -> PinState;
}