#pragma once

#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>

#include <cstring>


namespace sts1cobcsw
{
namespace ts = type_safe;
inline auto eduEnabledGpio = hal::GpioPin(hal::eduEnabledPin);

// extern RODOS::HAL_UART eduUart;
inline auto eduUart = RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);

/**
 * @brief Copy a value from a buffer to a variable.
 *
 * During the process, the position parameter is updated, so that one can chain
 * multiple calls to CopyFrom(). The size of the variable that must be copied from
 * the buffer is the size of the value parameter.
 *
 * @param buffer The buffer our data is copied from.
 * @param position The position in the buffer our data is copied from.
 * @param value The variable that will hold our copied value.
 */
template<std::size_t size>
auto CopyFrom(etl::string<size> const & buffer, ts::size_t * const position, auto * value)
{
    auto newPosition = *position + sizeof(*value);
    std::memcpy(value, &buffer[(*position).get()], sizeof(*value));
    *position = newPosition;
}

}
