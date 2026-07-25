#pragma once

#include <Sts1CobcSw/Firmware/Scheduler.hpp>

#include <Sts1CobcSw/RodosTime/RodosTime.hpp>

#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <algorithm>
#include <compare>


namespace sts1cobcsw
{
template<TaskVariantConcept T>
Scheduler<T>::Scheduler(std::span<ScheduledTask<T>> tasks) : tasks_(tasks)
{}


template<TaskVariantConcept T>
auto Scheduler<T>::Initialize() -> void
{
    for(auto & scheduledTask : tasks_)
    {
        std::visit([](auto & t) { t.Initialize(); }, scheduledTask.task);
    }
}


template<TaskVariantConcept T>
auto Scheduler<T>::Run() -> void
{
    while(true)
    {
        auto nextExecutionTime = endOfTime;
        for(auto & scheduledTask : tasks_)
        {
            // CurrentRodosTime() is re-read for every task meaning
            // a task which became due while a previous one was executing, runs in this sweep or
            // immediately in the next one
            if(scheduledTask.nextExecutionTime <= CurrentRodosTime())
            {
                std::visit([&scheduledTask](auto & t)
                           { scheduledTask.nextExecutionTime = t.Execute(); },
                           scheduledTask.task);
            }
            nextExecutionTime = std::min(nextExecutionTime, scheduledTask.nextExecutionTime);
        }
        // if a task became due in the meantime, this returns immediately
        SuspendUntil(nextExecutionTime);
    }
}
}
