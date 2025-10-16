#include <Sts1CobcSw/Firmware/EduPowerManagementThread.hpp>

#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/RealTime/RealTime.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <utility>


namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 6000U;
// Higher margin as the edu timedout, when a program is uploaded and started at the same time
constexpr auto eduBootTime = 20 * s;  // Measured ~19 s
constexpr auto eduBootTimeMargin = 40 * s;
constexpr auto eduPowerManagementThreadInterval = 2 * s;

auto epsBatteryGoodGpioPin = hal::GpioPin(hal::epsBatteryGoodPin);
RODOS::Semaphore semaphore{};
auto eduShouldBeReset = false;


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
        epsBatteryGoodGpioPin.ActivatePullUp();
    }


    // NOLINTNEXTLINE(*cognitive-complexity)
    void run() override
    {
        SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
        SuspendFor(eduPowerManagementThreadStartDelay);
        DEBUG_PRINT("Starting EDU power management thread\n");
        TIME_LOOP(0, value_of(eduPowerManagementThreadInterval))
        {
            {
                auto protector = RODOS::ScopeProtector(&semaphore);
                if(eduShouldBeReset)
                {
                    DEBUG_PRINT("Resetting EDU\n");
                    edu::TurnOff();
                    eduShouldBeReset = false;
                    continue;
                }
            }
            if(epsBatteryGoodGpioPin.Read() == hal::PinState::reset
               or not persistentVariables.Load<"flashIsWorking">())
            {
                DEBUG_PRINT(
                    "%s",
                    persistentVariables.Load<"eduShouldBePowered">() ? "Turning EDU off\n" : "");
                edu::TurnOff();
                continue;
            }
            auto eduIsAlive = false;
            eduIsAliveBufferForPowerManagement.get(eduIsAlive);
            auto nextEduProgramStartTime = RealTime(0);
            nextEduProgramStartTimeBuffer.get(nextEduProgramStartTime);
            auto timeTillNextEduProgram = ToRodosTime(nextEduProgramStartTime) - CurrentRodosTime();
            auto eduHasUpdate = edu::updateGpioPin.Read() == hal::PinState::set;
            if(eduIsAlive)
            {
                auto noWorkMustBeDoneInTheNearFuture =
                    not eduHasUpdate and not edu::ProgramsAreAvailableOnCobc()
                    and timeTillNextEduProgram > persistentVariables.Load<"maxEduIdleDuration">();
                if(noWorkMustBeDoneInTheNearFuture)
                {
                    DEBUG_PRINT("Turning EDU off\n");
                    edu::TurnOff();
                }
            }
            else if(not eduIsAlive and timeTillNextEduProgram < (eduBootTime + eduBootTimeMargin))
            {
                DEBUG_PRINT("Turning EDU on\n");
                edu::TurnOn();
            }
            DEBUG_PRINT_STACK_USAGE();
        }
    }
} eduPowerManagementThread;
}


auto ResetEdu() -> void
{
    {
        auto protector = RODOS::ScopeProtector(&semaphore);
        eduShouldBeReset = true;
    }
    // Wait long enough to ensure that the power management thread has a chance to reset the EDU
    SuspendFor(eduPowerManagementThreadInterval);
}
}
