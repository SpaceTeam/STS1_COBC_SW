#include <Sts1CobcSw/CobcSoftware/EduListenerThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/CobcSoftware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/FileSystem/DirectoryIterator.hpp>
#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <algorithm>


namespace sts1cobcsw
{
namespace
{
// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 2000U;
// TODO: Come up with the "right" numbers
constexpr auto eduBootTime = 20 * s;  // Measured ~19 s
constexpr auto eduBootTimeMargin = 5 * s;
constexpr auto eduPowerManagementThreadStartDelay = 15 * s;
constexpr auto eduPowerManagementThreadPeriod = 2 * s;
constexpr auto startDelayLimit = 60 * s;

auto epsBatteryGoodGpioPin = hal::GpioPin(hal::epsBatteryGoodPin);
}


class EduPowerManagementThread : public RODOS::StaticThread<stackSize>
{
public:
    EduPowerManagementThread()
        : StaticThread("EduPowerManagementThread", eduPowerManagementThreadPriority)
    {}


private:
    void init() override
    {
        epsBatteryGoodGpioPin.SetDirection(hal::PinDirection::in);
    }


    void run() override
    {
        TIME_LOOP(value_of(eduPowerManagementThreadStartDelay),
                  value_of(eduPowerManagementThreadPeriod))
        {
            auto batteryIsGood = epsBatteryGoodGpioPin.Read() == hal::PinState::set;
            auto flashIsWorking = persistentVariables.template Load<"flashIsWorking">();
            if(batteryIsGood and flashIsWorking)
            {
                auto eduIsAlive = false;
                eduIsAliveBufferForPowerManagement.get(eduIsAlive);
                auto startDelay = Duration(0);
                nextProgramStartDelayBuffer.get(startDelay);
                auto eduHasUpdate = eduUpdateGpioPin.Read() == hal::PinState::set;
                auto noWorkMustBeDoneInTheNearFuture = not eduHasUpdate
                                                   and not edu::ProgramsAreAvailableOnCobc()
                                                   and startDelay >= startDelayLimit;
                if(eduIsAlive and noWorkMustBeDoneInTheNearFuture)
                {
                    DEBUG_PRINT("Turning EDU off\n");
                    edu::TurnOff();
                }
                else if(not eduIsAlive and startDelay < (eduBootTime + eduBootTimeMargin))
                {
                    DEBUG_PRINT("Turning EDU on\n");
                    edu::TurnOn();
                }
            }
            else
            {
                DEBUG_PRINT("Turning EDU off\n");
                edu::TurnOff();
            }
        }
    }
} eduPowerManagementThread;
}
