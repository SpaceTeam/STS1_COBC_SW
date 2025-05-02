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

namespace fs = sts1cobcsw::fs;


namespace sts1cobcsw
{
// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 2'000U;
// TODO: Come up with the "right" numbers
constexpr auto eduBootTime = 20 * s;  // Measured ~19 s
constexpr auto eduBootTimeMargin = 5 * s;
constexpr auto eduPowerManagementThreadPeriod = 2 * s;
constexpr auto startDelayLimit = 60 * s;

// TODO: There should be an Eps.hpp/.cpp for this
auto epsBatteryGoodGpioPin = hal::GpioPin(hal::epsBatteryGoodPin);


auto HasStoredEduPrograms() -> bool
{
    auto iteratorResult = fs::MakeIterator(fs::Path("programs/"));
    if(iteratorResult.has_error())
    {
        return false;
    }

    auto iterator = iteratorResult.value();

    // Check if there's at least one file in the directory
    for(auto const & entryResult : iterator)
    {
        if(entryResult.has_error())
        {
            continue;
        }

        auto const & entry = entryResult.value();
        if(entry.type == fs::EntryType::file)
        {
            return true;
        }
    }

    // No program files found
    return false;
}


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
        epsBatteryGoodGpioPin.SetDirection(hal::PinDirection::in);
    }


    void run() override
    {
        TIME_LOOP(0, value_of(eduPowerManagementThreadPeriod))
        {
            // DEBUG_PRINT("[EduPowerManagementThread] Start of Loop\n");
            auto startDelay = Duration(0);
            nextProgramStartDelayBuffer.get(startDelay);

            auto const epsBatteryIsGood = epsBatteryGoodGpioPin.Read() == hal::PinState::set;
            auto const eduHasUpdate = eduUpdateGpioPin.Read() == hal::PinState::set;

            auto eduIsAlive = false;
            eduIsAliveBufferForPowerManagement.get(eduIsAlive);
            auto flashIsWorking = persistentVariables.template Load<"flashIsWorking">();

            // enough power and filesystem is working
            if(epsBatteryIsGood and flashIsWorking)
            {
                // Does edu heart beats ?
                if(eduIsAlive)
                {
                    // TODO: also perform a check about EDU programs on cobc
                    if((not eduHasUpdate) and (startDelay >= startDelayLimit)
                       and (not HasStoredEduPrograms()))
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
                DEBUG_PRINT("Turning Edu off\n");
                edu::TurnOff();
            }
        }
    }
} eduPowerManagementThread;
}
