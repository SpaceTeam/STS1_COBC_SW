//! @file
//! @brief  Manages the power of the EDU module

#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>
#include <Sts1CobcSw/Periphery/PersistentState.hpp>
#include <Sts1CobcSw/TopicsAndSubscribers.hpp>

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>

#include <algorithm>

namespace sts1cobcsw
{
namespace ts = type_safe;


// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 2'000U;
// TODO: Come up with the "right" numbers
constexpr auto eduBootTime = 2 * RODOS::SECONDS;
constexpr auto eduBootTimeMargin = 5 * RODOS::SECONDS;
constexpr auto startDelayLimit = 60 * RODOS::SECONDS;

auto epsBatteryGoodGpioPin = hal::GpioPin(hal::epsBatteryGoodPin);
// TODO: Move to Edu.hpp/cpp
auto eduHasUpdateGpioPin = hal::GpioPin(hal::eduUpdatePin);


class EduPowerManagementThread : public RODOS::StaticThread<stackSize>
{
    void init() override
    {
        epsBatteryGoodGpioPin.Direction(hal::PinDirection::in);
        eduHasUpdateGpioPin.Direction(hal::PinDirection::in);

        periphery::persistentstate::Initialize();
    }


    void run() override
    {
        // TODO : Get this value from edu queue (this will also impact startDelay Computation).
        auto const startTime = RODOS::NOW() + 20 * RODOS::SECONDS;

        ts::bool_t epsBatteryIsGood = epsBatteryGoodGpioPin.Read() == hal::PinState::set;
        ts::bool_t eduHasUpdate = eduHasUpdateGpioPin.Read() == hal::PinState::set;

        auto eduIsAlive = false;
        eduIsAliveBuffer.get(eduIsAlive);

        auto startDelay = std::max(startTime - RODOS::NOW(), 0 * RODOS::SECONDS);

        if(epsBatteryIsGood)
        {
            if(eduIsAlive)
            {
                // TODO: also perform a check about archives on cobc
                if(not(eduHasUpdate or startDelay < startDelayLimit))
                {
                    edu.TurnOff();
                }
            }
            else
            {
                if(startDelay < (eduBootTime + eduBootTimeMargin))
                {
                    edu.TurnOn();
                }
            }
        }
        else
        {
            edu.TurnOff();
        }

        // TODO: Give the 2 seconds a name
        RODOS::AT(RODOS::NOW() + 2 * RODOS::SECONDS);
    }
} eduPowerManagementThread;
}
