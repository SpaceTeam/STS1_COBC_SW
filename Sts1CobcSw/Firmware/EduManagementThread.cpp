#include <Sts1CobcSw/Firmware/EduPowerManagementThread.hpp>
#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Scheduler/Scheduler.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>


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
    static constexpr auto startTime = RodosTime(0) + totalStartupTestTimeout;

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
    static constexpr auto startTime =
        RodosTime(0) + totalStartupTestTimeout + eduPowerManagementThreadStartDelay;

    [[nodiscard]] auto Execute() -> RodosTime
    {
        // TODO: move TIME_LOOP body of EduProgramTransferThread::run() here
        return CurrentRodosTime() + eduProgramTransferTaskInterval;
    }
};


class EduPowerManagementTask
{
public:
    static constexpr auto startTime =
        RodosTime(0) + totalStartupTestTimeout + eduPowerManagementThreadStartDelay;

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


static_assert(ATask<EduPowerManagementTask>);
static_assert(ATask<EduListenerTask>);
static_assert(ATask<EduProgramTransferTask>);

auto scheduler = Scheduler<EduPowerManagementTask, EduListenerTask, EduProgramTransferTask>{};


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
