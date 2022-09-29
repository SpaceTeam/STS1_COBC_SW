#pragma once

#include "Io.hpp"

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
// TODO: Why is this here? Wouldn't it fit better in CobcCommands.hpp?
inline auto eduEnabledGpio = RODOS::HAL_GPIO(eduEnabledPin);

inline auto eduUart = RODOS::HAL_UART(eduUartIndex, eduUartTxPin, eduUartRxPin);
}
