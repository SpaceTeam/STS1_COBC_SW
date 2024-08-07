#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>

#include <Sts1CobcSw/Hal/GpioPin.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
class DisableRfLatchupProtectionTest : public RODOS::StaticThread<>
{
    void init() override
    {
#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Direction(hal::PinDirection::out);
#endif
    }


    void run() override
    {
#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Set();
#endif
    }
} disableRfLatchupProtectionTest;
}
