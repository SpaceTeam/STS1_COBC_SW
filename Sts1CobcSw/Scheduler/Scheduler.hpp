#pragma once


#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <array>
#include <concepts>
#include <tuple>


namespace sts1cobcsw
{
namespace internal
{
// tasks that don't have hardware to setup don't define Initialize()
template<typename T>
concept HasInitialize = requires(T & task) {
    { task.Initialize() } -> std::same_as<void>;
};
}


template<typename T>
concept ATask = std::default_initializable<T> and requires(T & task) {
    { T::startTime } -> std::convertible_to<RodosTime>;
    { task.Execute() } -> std::same_as<RodosTime>;
};

// a task returning a time in the past would turn Run() into a loop,
// which would starve the lower priority watchdog thread and therefore reset the COBC
constexpr auto minimumTaskInterval = 1 * ms;


template<ATask... Tasks>
    requires(sizeof...(Tasks) > 0)
class Scheduler
{
public:
    auto Initialize() -> void;
    [[noreturn]] auto Run() -> void;
    // public so a single sweep can be unit tested without the endless loop in Run()
    [[nodiscard]] auto ExecuteDueTasks() -> RodosTime;
    template<typename Task>
    [[nodiscard]] auto Get() -> Task &;


private:
    std::tuple<Tasks...> tasks_;
    std::array<RodosTime, sizeof...(Tasks)> nextExecutionTimes_ = {Tasks::startTime...};
};
}

#include <Sts1CobcSw/Scheduler/Scheduler.ipp>  // IWYU pragma: keep
