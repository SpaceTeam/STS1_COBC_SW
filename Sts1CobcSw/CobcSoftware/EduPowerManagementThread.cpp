//! @file
//! @brief  Manages the power of the EDU module
#include <Sts1CobcSw/CobcSoftware/EduListenerThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/CobcSoftware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/PersistentState.hpp>
#include <Sts1CobcSw/Utility/Debug.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 2'000U;
// TODO: Come up with the "right" numbers
constexpr auto eduBootTime = 20 * RODOS::SECONDS;  // Measured ~19 s
constexpr auto eduPowerManagementThreadPeriod = 2 * RODOS::SECONDS;
constexpr auto eduBootTimeMargin = 5 * RODOS::SECONDS;
constexpr auto startDelayLimit = 60 * RODOS::SECONDS;

// TODO: There should be an Eps.hpp/.cpp for this
auto epsBatteryGoodGpioPin = hal::GpioPin(hal::epsBatteryGoodPin);


class EduPowerManagementThread : public RODOS::StaticThread<stackSize>
{
public:
    EduPowerManagementThread()
        : StaticThread("EduPowerManagementThread", eduPowerManagementThreadPriority)
    {
    }

private:
    void init() override
    {
        epsBatteryGoodGpioPin.Direction(hal::PinDirection::in);
        persistentstate::Initialize();
    }


    void run() override
    {
        TIME_LOOP(0, eduPowerManagementThreadPeriod)
        {
            // DEBUG_PRINT("[EduPowerManagementThread] Start of Loop\n");
            std::int64_t startDelay = 0;
            nextProgramStartDelayBuffer.get(startDelay);

            auto const epsBatteryIsGood = epsBatteryGoodGpioPin.Read() == hal::PinState::set;
            auto const eduHasUpdate = eduUpdateGpioPin.Read() == hal::PinState::set;

            auto eduIsAlive = false;
            eduIsAliveBufferForPowerManagement.get(eduIsAlive);


            if(epsBatteryIsGood)
            {
                if(eduIsAlive)
                {
                    // TODO: also perform a check about EDU programs on cobc
                    if((not eduHasUpdate) and (startDelay >= startDelayLimit))
                    {
                        DEBUG_PRINT("Turning Edu off\n");
                        edu::TurnOff();
                    }
                }
                else
                {
                    if(startDelay < (eduBootTime + eduBootTimeMargin))
                    {
                        DEBUG_PRINT("Turning Edu on\n");
                        edu::TurnOn();
                    }
                }
            }
            else
            {
                edu::TurnOff();
            }
        }
    }
} eduPowerManagementThread;
}
