#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>

#include <array>


namespace sts1cobcsw
{
// pa2, pa3 are automatically tested by printing to UCI UART (PRINTF)
auto pinsToTest = std::to_array<hal::GpioPin>(
    {hal::pa1,  hal::pa5,  hal::pa6,  hal::pa7,  hal::pa8,  hal::pa9,  hal::pa10, hal::pa11,
     hal::pa12, hal::pa13, hal::pa14, hal::pa15, hal::pb0,  hal::pb1,  hal::pb3,  hal::pb4,
     hal::pb5,  hal::pb6,  hal::pb7,  hal::pb8,  hal::pb9,  hal::pb12, hal::pb13, hal::pb14,
     hal::pb15, hal::pc0,  hal::pc1,  hal::pc2,  hal::pc3,  hal::pc4,  hal::pc5,  hal::pc7,
     hal::pc9,  hal::pc10, hal::pc11, hal::pc12, hal::pc13, hal::pd2});


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
                RODOS::PRINTF("Current pin set to %s \n", (toggle ? "true" : "false"));
                RODOS::PRINTF("Current pin reads %s \n",
                              (pin.Read() == hal::PinState::set ? "true" : "false"));
            }
            toggle = not toggle;
        }
    }
};


auto const gpioTest = GpioTest();
}
