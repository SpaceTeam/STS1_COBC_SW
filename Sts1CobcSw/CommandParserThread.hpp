#pragma once


#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>


namespace sts1cobcsw
{
// FIXME: No! This must not be necessary here. High-level code only calls the public Edu API. Also,
// this shouldn't be inline in any case.
inline auto eduEnabledGpioPin = hal::GpioPin(hal::eduEnabledPin);
}
