//! @file
//! @brief  Manages the power of the EDU module

#include <Sts1CobcSw/Hal/Gpio.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>
#include <type_safe/types.hpp>

#include <rodos.h>

#include <algorithm>

namespace sts1cobcsw
{
namespace ts = type_safe;

auto epsBatteryGoodGpio = HAL_GPIO(hal::epsBatteryGoodPin);
auto eduHasUpdateGpio = HAL_GPIO(hal::eduUpdatePin);

// TODO : move this in another file like in rpg
RODOS::Topic<bool> eduIsAliveTopic(-1, "eduHeartBeatsTopic");
auto eduIsAliveBuffer = CommBuffer<bool>();
auto eduIsAliveSubscriber = Subscriber(eduIsAliveTopic, eduIsAliveBuffer, "eduIsAliveSubscriber");

class EduPowerManagementThread : public StaticThread<>
{
    void init() override
    {
        hal::InitPin(epsBatteryGoodGpio, hal::PinType::input, hal::PinVal::zero);
        hal::InitPin(eduHasUpdateGpio, hal::PinType::input, hal::PinVal::zero);
    }

    void run() override
    {
        auto const startTime = RODOS::NOW();

        ts::bool_t const epsBatteryIsGood = epsBatteryGoodGpio.readPins() != 0;
        ts::bool_t const eduHasUpdate = eduHasUpdateGpio.readPins() != 0;

        auto eduIsAlive = false;
        eduIsAliveBuffer.get(eduIsAlive);
        auto delayTime = std::max(startTime - RODOS::NOW(), 0 * MILLISECONDS);

        constexpr auto eduBootTime = 2 * SECONDS;
        constexpr auto eduBootTimeMargin = 5 * SECONDS;

        if(epsBatteryIsGood)
        {
            if(eduIsAlive)
            {
                if(true /*not (eduHasUpdate or delayTime < 60 * SECONDS or any archives on cobc) */)
                {
                    PRINTF("TURN OFF EDU");
                }
            }
            else
            {
                if(delayTime < eduBootTime + eduBootTimeMargin)
                {
                    PRINTF("TURN ON EDU");
                }
            }
        }
        else
        {
            PRINTF("TURN OFF EDU");
        }

        RODOS::AT(NOW() + 2 * MILLISECONDS);
    }
};
auto const eduPowerManagementThread = EduPowerManagementThread();
}
