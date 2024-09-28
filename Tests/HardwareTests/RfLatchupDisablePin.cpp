#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>

#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>


namespace sts1cobcsw
{
#if HW_VERSION >= 27 and HW_VERSION < 30
auto rfLatchupDisableGpioPin = hal::GpioPin(hal::rfLatchupDisablePin);
#endif
#if HW_VERSION >= 30
auto rfLatchupDisableGpioPin1 = hal::GpioPin(hal::rfLatchupDisablePin1);
auto rfLatchupDisableGpioPin2 = hal::GpioPin(hal::rfLatchupDisablePin2);
#endif


auto InitializeRfLatchupDisablePins() -> void
{
#if HW_VERSION >= 27 and HW_VERSION < 30
    rfLatchupDisableGpioPin.Direction(hal::PinDirection::out);
#endif
#if HW_VERSION >= 30
    rfLatchupDisableGpioPin1.Direction(hal::PinDirection::out);
    rfLatchupDisableGpioPin2.Direction(hal::PinDirection::out);
#endif
}


auto EnableRfLatchupProtection() -> void
{
#if HW_VERSION >= 27 and HW_VERSION < 30
    rfLatchupDisableGpioPin.Reset();
#endif
#if HW_VERSION >= 30
    rfLatchupDisableGpioPin1.Reset();
    rfLatchupDisableGpioPin2.Reset();
#endif
}


auto DisableRfLatchupProtection() -> void
{
#if HW_VERSION >= 27 and HW_VERSION < 30
    rfLatchupDisableGpioPin.Set();
#endif
#if HW_VERSION >= 30
    rfLatchupDisableGpioPin1.Set();
    rfLatchupDisableGpioPin2.Set();
#endif
}
}
