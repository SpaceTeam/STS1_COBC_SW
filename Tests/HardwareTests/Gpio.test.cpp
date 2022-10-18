#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>

#include <array>


namespace sts1cobcsw
{
// pa2, pa3 are automatically tested by printing to UCI UART (PRINTF)
// TODO: Find out which pins we can safely use here. The problem is that some pins cause a short and
// therefore a reset of the COBC when used in this test. The LED pin has to work for this test
// though.
auto pinsToTest = std::to_array<hal::GpioPin>({hal::ledPin});


class GpioTest : public RODOS::StaticThread<>
{
    void init() override
    {
        for(auto & pin : pinsToTest)
        {
            pin.Direction(hal::PinDirection::out);
        }
    }


    void run() override
    {
        type_safe::bool_t toggle = true;

        TIME_LOOP(0, 1000 * RODOS::MILLISECONDS)
        {
            for(auto & pin : pinsToTest)
            {
                toggle ? pin.Set() : pin.Reset();
                RODOS::PRINTF("Pin was %s and reads %s\n",
                              toggle ? "  set" : "reset",                             // NOLINT
                              pin.Read() == hal::PinState::set ? "  set" : "reset");  // NOLINT
            }
            toggle = not toggle;
        }
    }
};


auto const gpioTest = GpioTest();
}
