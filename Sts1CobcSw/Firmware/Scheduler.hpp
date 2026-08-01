#pragma once


#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <concepts>
#include <span>
#include <type_traits>
#include <variant>


namespace sts1cobcsw
{
template<typename T>
concept ExecutableTask = requires(T t) {
    { t.Initialize() } -> std::same_as<void>;
    { t.Execute() } -> std::same_as<RodosTime>;
};

template<typename T>
struct IsVariantOfTask : std::false_type
{};


template<typename... T>
    requires(ExecutableTask<T> && ...)
struct IsVariantOfTask<std::variant<T...>> : std::true_type
{};


template<typename T>
concept TaskVariantConcept = IsVariantOfTask<T>::value;


template<TaskVariantConcept T>
struct ScheduledTask
{
    T task;
    RodosTime nextExecutionTime = endOfTime;
};


template<TaskVariantConcept T>
class Scheduler
{
public:
    explicit Scheduler(std::span<ScheduledTask<T>> tasks);

    auto Initialize() -> void;
    [[noreturn]] auto Run() -> void;


private:
    std::span<ScheduledTask<T>> tasks_;
};
}

#include <Sts1CobcSw/Firmware/Scheduler.ipp>  // IWYU pragma: keep
