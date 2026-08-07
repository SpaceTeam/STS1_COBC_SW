#pragma once

#include <Sts1CobcSw/Firmware/Scheduler.hpp>

#include <Sts1CobcSw/RodosTime/RodosTime.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <algorithm>
#include <compare>
#include <cstddef>
#include <tuple>
#include <utility>


namespace sts1cobcsw
{
template<ATask... Tasks>
    requires(sizeof...(Tasks) > 0)
auto Scheduler<Tasks...>::Initialize() -> void
{
    std::apply(
        [](auto &... task)
        {
            auto InitializeIfNecessary = [](auto & t)
            {
                if constexpr(internal::HasInitialize<decltype(t)>)
                {
                    t.Initialize();
                }
            };
            (InitializeIfNecessary(task), ...);
        },
        tasks_);
}


template<ATask... Tasks>
    requires(sizeof...(Tasks) > 0)
auto Scheduler<Tasks...>::Run() -> void
{
    while(true)
    {
        SuspendUntil(ExecuteDueTasks());
    }
}


template<ATask... Tasks>
    requires(sizeof...(Tasks) > 0)
auto Scheduler<Tasks...>::ExecuteDueTasks() -> RodosTime
{
    auto nextExecutionTime = endOfTime;
    [&]<std::size_t... i>(std::index_sequence<i...>)
    {
        (
            [&]
            {
                auto now = CurrentRodosTime();
                if(nextExecutionTimes_[i] <= now)
                {
                    nextExecutionTimes_[i] =
                        std::max(std::get<i>(tasks_).Execute(), now + minimumTaskInterval);
                }
                nextExecutionTime = std::min(nextExecutionTime, nextExecutionTimes_[i]);
            }(),
            ...);
    }(std::index_sequence_for<Tasks...>{});
    return nextExecutionTime;
}


template<ATask... Tasks>
    requires(sizeof...(Tasks) > 0)
template<typename Task>
auto Scheduler<Tasks...>::Get() -> Task &
{
    return std::get<Task>(tasks_);
}
}
