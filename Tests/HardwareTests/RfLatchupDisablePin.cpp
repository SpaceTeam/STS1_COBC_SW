#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>

#include <Sts1CobcSw/Hal/IoNames.hpp>


namespace sts1cobcsw
{
#if HW_VERSION >= 27
hal::GpioPin rfLatchupDisableGpioPin(hal::rfLatchupDisablePin);
#endif
}