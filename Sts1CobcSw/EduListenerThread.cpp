#include <Sts1CobcSw/Dummy.hpp>

#include <Sts1CobcSw/Hal/Gpio.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>

#include <rodos.h>


namespace sts1cobcsw
{
auto eduUpdateGpio = HAL_GPIO(hal::eduUpdatePin);

class EduListenerThread : public StaticThread<>
{
    void init() override
    {
        hal::InitPin(eduUpdateGpio, hal::PinType::input, hal::PinVal::one);
    }

    void run() override
    {
        if(true /*eduHasUpdate*/) {
            // TODO Communicate with EDU
        }

        AT(NOW() + 1 * SECONDS);
    }
};

auto const eduListenerThread = EduListenerThread();
}
