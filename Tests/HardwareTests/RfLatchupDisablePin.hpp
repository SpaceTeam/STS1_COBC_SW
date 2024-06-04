#pragma once

#include <Sts1CobcSw/Hal/GpioPin.hpp>


namespace sts1cobcsw
{
#if HW_VERSION >= 27
extern hal::GpioPin rfLatchupDisableGpioPin;
#endif
}
