#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <utility>


namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 1000U;
constexpr auto antennaDeploymentDelay = 5 * min;
constexpr auto antennaDeploymentHeatDuration = 10 * s;

auto antennaDeploymentPin = hal::GpioPin(hal::antennaDeploymentPin);


class AntennaDeploymentThread : public RODOS::StaticThread<stackSize>
{
public:
    AntennaDeploymentThread()
        : StaticThread("AntennaDeploymentThread", antennaDeploymentThreadPriority)
    {}


private:
    void init() override
    {
        antennaDeploymentPin.SetDirection(hal::PinDirection::out);
    }


    void run() override
    {
        SuspendFor(antennaDeploymentDelay);
        DEBUG_PRINT("Starting antenna deployment thread\n");
        if(persistentVariables.Load<"antennasShouldBeDeployed">())
        {
            DEBUG_PRINT("Start deploying antenna\n");
            antennaDeploymentPin.Set();
            SuspendFor(antennaDeploymentHeatDuration);
            antennaDeploymentPin.Reset();
        }
        DEBUG_PRINT_STACK_USAGE();
        SuspendUntil(endOfTime);
    }
} antennaDeploymentThread;
}
}
