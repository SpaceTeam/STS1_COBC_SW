//! @file
//! @brief  Manages the power of the EDU module

#include <Sts1CobcSw/EduListenerThread.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>
#include <Sts1CobcSw/Periphery/PersistentState.hpp>
#include <Sts1CobcSw/TopicsAndSubscribers.hpp>

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <cinttypes>


namespace sts1cobcsw
{
namespace ts = type_safe;


// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 2'000U;
// TODO: Come up with the "right" numbers
constexpr auto eduBootTime = 20 * RODOS::SECONDS;  // Measured ~19 s
constexpr auto eduPowerManagementThreadDelay = 2 * RODOS::SECONDS;
constexpr auto eduBootTimeMargin = 5 * RODOS::SECONDS;
constexpr auto startDelayLimit = 60 * RODOS::SECONDS;
// TODO: Set this to 500
constexpr auto threadPriority = 500;

auto epsBatteryGoodGpioPin = hal::GpioPin(hal::epsBatteryGoodPin);
// TODO: Move to Edu.hpp/cpp


class EduPowerManagementThread : public RODOS::StaticThread<stackSize>
{
public:
    EduPowerManagementThread() : StaticThread("EduPowerManagementThread", threadPriority)
    {
    }

private:
    void init() override
    {
        epsBatteryGoodGpioPin.Direction(hal::PinDirection::in);

        periphery::persistentstate::Initialize();
    }


    void run() override
    {
        // RODOS::PRINTF("[EduPowerManagementThread] Start of Run()\n");

        TIME_LOOP(0, eduPowerManagementThreadDelay)
        {
            // RODOS::PRINTF("[EduPowerManagementThread] Start of Loop\n");
            std::int64_t startDelay = 0;
            nextProgramStartDelayBuffer.get(startDelay);

            ts::bool_t epsBatteryIsGood = epsBatteryGoodGpioPin.Read() == hal::PinState::set;
            ts::bool_t eduHasUpdate = eduUpdateGpioPin.Read() == hal::PinState::set;

            auto eduIsAlive = false;
            eduIsAliveBufferForPowerManagement.get(eduIsAlive);


            if(epsBatteryIsGood)
            {
                if(eduIsAlive)
                {
                    // TODO: also perform a check about archives on cobc
                    if(not(eduHasUpdate or startDelay < startDelayLimit))
                    {
                        RODOS::PRINTF("Turning Edu off\n");
                        edu.TurnOff();
                    }
                }
                else
                {
                    if(startDelay < (eduBootTime + eduBootTimeMargin))
                    {
                        RODOS::PRINTF("Turning Edu on\n");
                        edu.TurnOn();
                    }
                }
            }
            else
            {
                edu.TurnOff();
            }
        }
    }
} eduPowerManagementThread;
}
