//! @file
//! @brief  Manages the power of the EDU module

#include <Sts1CobcSw/CobcCommands.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>
#include <Sts1CobcSw/Periphery/PersistentState.hpp>
#include <Sts1CobcSw/Topics.hpp>

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>

#include <algorithm>

namespace sts1cobcsw
{
namespace ts = type_safe;


auto epsBatteryGoodGpio = hal::GpioPin(hal::epsBatteryGoodPin);
auto eduHasUpdateGpio = hal::GpioPin(hal::eduUpdatePin);
auto eduEnableGpio = hal::GpioPin(hal::eduEnabledPin);

constexpr auto eduBootTime = 2 * RODOS::SECONDS;
constexpr auto eduBootTimeMargin = 5 * RODOS::SECONDS;
constexpr auto startDelayLimit = 60 * RODOS::SECONDS;

auto eduIsAliveBuffer = RODOS::CommBuffer<bool>();
auto eduIsAliveSubscriber =
    RODOS::Subscriber(eduIsAliveTopic, eduIsAliveBuffer, "eduIsAliveSubscriber");


class EduPowerManagementThread : public RODOS::StaticThread<>
{
    void init() override
    {
        epsBatteryGoodGpio.Direction(hal::PinDirection::in);
        eduHasUpdateGpio.Direction(hal::PinDirection::in);
        eduEnableGpio.Direction(hal::PinDirection::out);

        periphery::persistentstate::Initialize();
    }


    void run() override
    {
        // TODO : Get this value from edu queue (this will also impact startDelay Computation).
        auto const startTime = RODOS::NOW() + 20 * RODOS::SECONDS;

        ts::bool_t epsBatteryIsGood = epsBatteryGoodGpio.Read() == hal::PinState::set;
        ts::bool_t eduHasUpdate = eduHasUpdateGpio.Read() == hal::PinState::set;

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
                if(startDelay < eduBootTime + eduBootTimeMargin)
                {
                    edu.TurnOn();
                }
            }
        }
        else
        {
            edu.TurnOff();
        }

        RODOS::AT(RODOS::NOW() + 2 * RODOS::SECONDS);
    }
};
auto const eduPowerManagementThread = EduPowerManagementThread();
}
