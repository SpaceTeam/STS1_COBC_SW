#include <Sts1CobcSw/Firmware/Scheduler.hpp>

#include <Sts1CobcSw/RodosTime/RodosTime.hpp>

#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <algorithm>
#include <compare>


namespace sts1cobcsw
{
auto Task::Initialize() -> void
{
    DoInitialize();
}


auto Task::Execute() -> RodosTime
{
    return DoExecute();
}


auto Task::DoInitialize() -> void
{}


Scheduler::Scheduler(std::span<ScheduledTask> tasks) : tasks_(tasks)
{}


auto Scheduler::Initialize() -> void
{
    for(auto & scheduledTask : tasks_)
    {
        scheduledTask.task->Initialize();
    }
}


auto Scheduler::Run() -> void
{
    while(true)
    {
        auto nextExecutionTime = endOfTime;
        for(auto & scheduledTask : tasks_)
        {
            // CurrentRodosTime() is re-read for every task meaning
            // a task which became due while a previous one was executing, runs in this sweep or immediately in the next one
            if(scheduledTask.nextExecutionTime <= CurrentRodosTime())
            {
                scheduledTask.nextExecutionTime = scheduledTask.task->Execute();
            }
            nextExecutionTime = std::min(nextExecutionTime, scheduledTask.nextExecutionTime);
        }
        // if a task became due in the meantime, this returns immediately
        SuspendUntil(nextExecutionTime);
    }
}
}
