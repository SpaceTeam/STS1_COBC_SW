#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>
#include <Sts1CobcSw/WatchdogTimers/WatchdogTimers.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <compare>
#include <cstdint>
#include <utility>


namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 1000U;
constexpr auto feedWatchdogIntervall = 800 * ms;
constexpr auto antennaDeploymentTime = RodosTime(5 * RODOS::MINUTES);
constexpr auto antennaDeploymentHeatDuration = 10 * s;

auto antennaDeploymentPin = hal::GpioPin(hal::antennaDeploymentPin);


enum class DeploymentStatus : std::uint8_t
{
    notStarted,
    inProgress,
    completed
};


class WatchdogAndAntennaDeploymentThread : public RODOS::StaticThread<stackSize>
{
public:
    WatchdogAndAntennaDeploymentThread()
        : StaticThread("WatchdogAndAntennaDeploymentThread",
                       watchdogAndAntennaDeploymentThreadPriority)
    {}


private:
    void init() override
    {
        antennaDeploymentPin.SetDirection(hal::PinDirection::out);
        antennaDeploymentPin.ActivatePullUp();
    }


    void run() override
    {
        SuspendFor(totalStartupTestTimeout);
        DEBUG_PRINT("Starting watchdog and antenna deployment thread\n");
#ifdef ENABLE_DEBUG_PRINT
        static constexpr auto printDelay = 100 * ms;  // Without this the UART buffer overflows
        SuspendFor(printDelay);
        DEBUG_PRINT("Deploying antenna in %lld s\n",
                    (antennaDeploymentTime - CurrentRodosTime()) / s);
#endif
        auto deploymentStatus = DeploymentStatus::notStarted;
        TIME_LOOP(0, value_of(feedWatchdogIntervall))
        {
            wdt::Feed();
            auto now = CurrentRodosTime();
            if(persistentVariables.Load<"antennasShouldBeDeployed">()
               and deploymentStatus == DeploymentStatus::notStarted
               and (now > antennaDeploymentTime))
            {
                DEBUG_PRINT("Start deploying antenna\n");
                antennaDeploymentPin.Set();
                deploymentStatus = DeploymentStatus::inProgress;
                DEBUG_PRINT_STACK_USAGE();
            }
            if(deploymentStatus == DeploymentStatus::inProgress
               and (now > antennaDeploymentTime + antennaDeploymentHeatDuration))
            {
                DEBUG_PRINT("Stop deploying antenna\n");
                antennaDeploymentPin.Reset();
                deploymentStatus = DeploymentStatus::completed;
                DEBUG_PRINT_STACK_USAGE();
            }
        }
    }
} watchdogAndAntennaDeploymentThread;
}
}
