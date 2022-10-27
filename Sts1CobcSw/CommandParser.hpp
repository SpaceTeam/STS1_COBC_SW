#pragma once


#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>

#include <cstring>


namespace sts1cobcsw
{
namespace ts = type_safe;
// FIXME: No! This must not be necessary here. High-level code only calls the public Edu API. Also,
// this shouldn't be inline in any case.
inline auto eduEnabledGpio = hal::GpioPin(hal::eduEnabledPin);

// extern RODOS::HAL_UART eduUart;

// FIXME: No! This must not be necessary here. High-level code only calls the public Edu API. Also,
// this shouldn't be inline in any case.
inline auto eduUart = RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);


//! @brief Copy a value from a buffer to a variable.
//!
//! During the process, the position parameter is updated, so that one can chain
//! multiple calls to CopyFrom(). The size of the variable that must be copied from
//! the buffer is the size of the value parameter.
//!
//! @param buffer The buffer our data is copied from.
//! @param position The position in the buffer our data is copied from.
//! @param value The variable that will hold our copied value.
// TODO: Remove this function. Use the serial library instead.
template<std::size_t size>
auto CopyFrom(etl::string<size> const & buffer, ts::size_t * const position, auto * value)
{
    auto newPosition = *position + sizeof(*value);
    std::memcpy(value, &buffer[(*position).get()], sizeof(*value));
    *position = newPosition;
}

}
