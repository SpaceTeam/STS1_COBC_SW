#pragma once


#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <span>


namespace sts1cobcsw
{

class Task
{
public:
    Task() = default;
    Task(Task const &) = delete;
    Task(Task &&) = delete;
    auto operator=(Task const &) -> Task & = delete;
    auto operator=(Task &&) -> Task & = delete;
    virtual ~Task() = default;

    auto Initialize() -> void;
    [[nodiscard]] auto Execute() -> RodosTime;


private:
    virtual auto DoInitialize() -> void;
    [[nodiscard]] virtual auto DoExecute() -> RodosTime = 0;
};


struct ScheduledTask
{
    Task * task = nullptr;
    RodosTime nextExecutionTime = endOfTime;
};

class Scheduler
{
public:
    explicit Scheduler(std::span<ScheduledTask> tasks);

    auto Initialize() -> void;
    [[noreturn]] auto Run() -> void;


private:
    std::span<ScheduledTask> tasks_;
};
}