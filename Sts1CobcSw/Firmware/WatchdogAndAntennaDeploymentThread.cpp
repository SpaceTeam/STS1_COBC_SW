#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
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

#include <etl/function.h>
#include <etl/vector.h>

#include <cstdint>
#include <utility>


namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 1000U;
constexpr auto antennaDeploymentDelay = 5 * min;
constexpr auto antennaDeploymentHeatDuration = 10 * s;
constexpr auto threadPeriode = 200 * ms;
constexpr auto maxNTasks = 2U;

auto antennaDeploymentPin = hal::GpioPin(hal::antennaDeploymentPin);
auto semaphore = RODOS::Semaphore();


struct TaskMetaData
{
    etl::function<void, void> taskFunction;
    bool callPeriodic = false;
    bool active = false;
    uint32_t intervalUnits = 0U;
    uint32_t unitsSinceLastExecution = 0U;
};


auto DeployAntennaTask() -> void;
auto FinishDeployAntennaTask() -> void;


class WatchdogAndAntennaDeploymentThread : public RODOS::StaticThread<stackSize>
{
public:
    WatchdogAndAntennaDeploymentThread()
        : StaticThread("WatchdogAndAntennaDeploymentThread",
                       watchdogAndAntennaDeploymentThreadPriority)
    {}


    void RegisterTask(etl::function<void, void> task, bool periodic, Duration const callIntervall)
    {
        TaskMetaData taskData = {
            std::move(task), periodic, true, static_cast<uint32_t>(callIntervall / threadPeriode)};
        if(tasks_.size() > maxNTasks)
        {
            // TODO: handle to many tasks added
            DEBUG_PRINT("Too many tasks added!\n");
        }
        else
        {
            semaphore.enter();
            tasks_.push_back(taskData);
            semaphore.leave();
        }
    }


private:
    etl::vector<TaskMetaData, maxNTasks> tasks_;


    void init() override
    {
        antennaDeploymentPin.SetDirection(hal::PinDirection::out);
        RegisterTask(etl::function<void, void>(DeployAntennaTask), false, antennaDeploymentDelay);
    }


    void run() override
    {
        SuspendFor(totalStartupTestTimeout);
        DEBUG_PRINT("Starting watchdog and antenna deployment thread\n");
        DEBUG_PRINT_STACK_USAGE();

        TIME_LOOP(0, value_of(threadPeriode))
        {
            semaphore.enter();
            for(auto task : tasks_)
            {
                if(not task.active)
                {
                    continue;
                }

                task.unitsSinceLastExecution++;
                if(task.unitsSinceLastExecution >= task.intervalUnits)
                {
                    task.taskFunction();
                    task.unitsSinceLastExecution = 0;
                    task.active = task.callPeriodic;
                }
            }
            semaphore.leave();
        }
    }
} watchdogAndAntennaDeploymentThread;


auto DeployAntennaTask() -> void
{
    if(persistentVariables.Load<"antennasShouldBeDeployed">())
    {
        DEBUG_PRINT("Start deploying antenna\n");
        antennaDeploymentPin.Set();
        watchdogAndAntennaDeploymentThread.RegisterTask(
            etl::function<void, void>(FinishDeployAntennaTask),
            false,
            antennaDeploymentHeatDuration);
    }
}


auto FinishDeployAntennaTask() -> void
{
    antennaDeploymentPin.Reset();
}
}
}
