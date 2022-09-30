#pragma once

#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
// TODO: Why is this here? Wouldn't it fit better in CobcCommands.hpp?
inline auto eduEnabledGpio = RODOS::HAL_GPIO(hal::eduEnabledPin);

//extern RODOS::HAL_UART eduUart;
inline auto eduUart = RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);
}
