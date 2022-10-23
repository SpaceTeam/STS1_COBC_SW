#pragma once

#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
inline auto eduEnabledGpio = hal::GpioPin(hal::eduEnabledPin);

// extern RODOS::HAL_UART eduUart;
inline auto eduUart = RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);
}
