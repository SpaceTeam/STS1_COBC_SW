#include <Sts1CobcSw/Firmware/EduPowerManagementThread.hpp>
#include <Sts1CobcSw/Firmware/Scheduler.hpp>
#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <span>


namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 6000U;
// TODO: move constants here from the old thread files
constexpr auto eduListenerTaskInterval = 1 * s;
constexpr auto eduProgramTransferTaskInterval = 5 * s;
constexpr auto eduPowerManagementTaskInterval = 2 * s;


class EduListenerTask
{
public:
    auto Initialize() -> void
    {
        // TODO: move init() code of EduListenerThread here (update and dosi enable GPIO setup)
    }


    [[nodiscard]] auto Execute() -> RodosTime
    {
        // TODO: move loop body of EduListenerThread::run() here.
        // polling loop in SuspendUntilEduIsAliveAndHasUpdate() dissolves into:
        // if EDU not alive or no update, just return and try again one interval later
        return CurrentRodosTime() + eduListenerTaskInterval;
    }
};


class EduProgramTransferTask
{
public:
    auto Initialize() -> void
    {
        // TODO: move init() code of EduListenerThread here (update and dosi enable GPIO setup)
    }

    [[nodiscard]] auto Execute() -> RodosTime
    {
        // TODO: move TIME_LOOP body of EduProgramTransferThread::run() here
        return CurrentRodosTime() + eduProgramTransferTaskInterval;
    }
};


class EduPowerManagementTask
{
public:
    auto Initialize() -> void
    {
        // TODO: move init() code of EduPowerManagementThread here
    }


    [[nodiscard]] auto Execute() -> RodosTime
    {
        // TODO: move TIME_LOOP body of EduPowerManagementThread::run() here
        // including ResetEdu() flag handling; every 'continue' becomes a 'return'
        return CurrentRodosTime() + eduPowerManagementTaskInterval;
    }
};


static_assert(ExecutableTask<EduListenerTask> && ExecutableTask<EduProgramTransferTask>
              && ExecutableTask<EduPowerManagementTask>);
using TaskVariant = std::variant<EduListenerTask, EduProgramTransferTask, EduPowerManagementTask>;

// array order determines the execution order of tasks that are due at the same time
// power management task runs first so that a requested EDU reset happens before further
// communication attempts
auto scheduledTasks = std::array{
    ScheduledTask<TaskVariant>{.task = EduListenerTask{},
                               .nextExecutionTime = RodosTime(0) + totalStartupTestTimeout
                                                  + eduPowerManagementThreadStartDelay    },
    ScheduledTask<TaskVariant>{.task = EduProgramTransferTask{},
                               .nextExecutionTime = RodosTime(0) + totalStartupTestTimeout},
    ScheduledTask<TaskVariant>{.task = EduPowerManagementTask{},
                               .nextExecutionTime = RodosTime(0) + totalStartupTestTimeout
                                                  + eduPowerManagementThreadStartDelay    },
};


auto scheduler = Scheduler<TaskVariant>(std::span(scheduledTasks));


class EduManagementThread : public RODOS::StaticThread<stackSize>
{
public:
    EduManagementThread() : StaticThread("EduManagementThread", eduManagementThreadPriority)
    {}


private:
    void init() override
    {
        scheduler.Initialize();
    }


    void run() override
    {
        scheduler.Run();
    }
} eduManagementThread;
}
}
