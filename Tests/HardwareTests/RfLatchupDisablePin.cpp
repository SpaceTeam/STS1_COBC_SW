#include <Sts1CobcSw/Hal/IoNames.hpp>

#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>


namespace sts1cobcsw
{
#if HW_VERSION >= 27
hal::GpioPin rfLatchupDisableGpioPin(hal::rfLatchupDisablePin);
#endif
}