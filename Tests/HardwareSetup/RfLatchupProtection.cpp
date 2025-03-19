#include <Tests/HardwareSetup/RfLatchupProtection.hpp>

#ifndef __linux__
    #include <Sts1CobcSw/Hal/GpioPin.hpp>
    #include <Sts1CobcSw/Hal/IoNames.hpp>
#endif


namespace sts1cobcsw
{
namespace
{
#ifndef __linux__
    #if HW_VERSION >= 27 and HW_VERSION < 30
auto rfLatchupDisableGpioPin = hal::GpioPin(hal::rfLatchupDisablePin);
    #endif
    #if HW_VERSION >= 30
auto rfLatchupDisableGpioPin1 = hal::GpioPin(hal::rfLatchupDisablePin1);
auto rfLatchupDisableGpioPin2 = hal::GpioPin(hal::rfLatchupDisablePin2);
    #endif
#endif
}


auto InitializeRfLatchupDisablePins() -> void
{
#ifndef __linux__
    #if HW_VERSION >= 27 and HW_VERSION < 30
    rfLatchupDisableGpioPin.SetDirection(hal::PinDirection::out);
    #endif
    #if HW_VERSION >= 30
    rfLatchupDisableGpioPin1.SetDirection(hal::PinDirection::out);
    rfLatchupDisableGpioPin2.SetDirection(hal::PinDirection::out);
    #endif
#endif
}


auto EnableRfLatchupProtection() -> void
{
#ifndef __linux__
    #if HW_VERSION >= 27 and HW_VERSION < 30
    rfLatchupDisableGpioPin.Reset();
    #endif
    #if HW_VERSION >= 30
    rfLatchupDisableGpioPin1.Reset();
    rfLatchupDisableGpioPin2.Reset();
    #endif
#endif
}


auto DisableRfLatchupProtection() -> void
{
#ifndef __linux__
    #if HW_VERSION >= 27 and HW_VERSION < 30
    rfLatchupDisableGpioPin.Set();
    #endif
    #if HW_VERSION >= 30
    rfLatchupDisableGpioPin1.Set();
    rfLatchupDisableGpioPin2.Set();
    #endif
#endif
}
}
