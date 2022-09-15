#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>

#include <array>


namespace sts1cobcsw
{
// TODO: Define proper names in Hal/PinNames.hpp and Hal/IoNames.hpp
auto pinsToTest = std::to_array<RODOS::HAL_GPIO>({RODOS::GPIO_037});


class GpioTest : public RODOS::StaticThread<>
{
    void init() override
    {
        for(auto & pin : pinsToTest)
        {
            // TODO: Provide a better abstraction for initializing pins. Boolean options/flags are
            // terrible. Use enum classes instead.
            pin.init(/*isOutput=*/true, 1, 0);
        }
    }

    void run() override
    {
        type_safe::bool_t toggle = true;

        TIME_LOOP(0, 100 * RODOS::MILLISECONDS)
        {
            for(auto & i : pinsToTest)
            {
                // TODO: Provide a better abstraction for setting and reading single pins. That cast
                // is just awful. I think an overload vor bool, type_safe::bool_t and maybe an enum
                // (class) would be a good idea.
                i.setPins(static_cast<uint32_t>(bool(toggle)));
            }
            toggle = not toggle;
        }
    }
};


auto const gpioTest = GpioTest();
}