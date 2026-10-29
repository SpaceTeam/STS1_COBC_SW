#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Scheduler/Scheduler.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>


namespace sts1cobcsw
{
namespace
{
auto nExecutedTasks = 0;


// DueTask<1> wants to run again in 1h, DueTask<2> in 2h, and so on
template<int id>
class DueTask
{
public:
    static constexpr auto startTime = RodosTime(0);
    int nExecutions = 0;
    int executionNumber = 0;
    RodosTime returnedTime = RodosTime(0);

    auto Execute() -> RodosTime
    {
        ++nExecutions;
        executionNumber = ++nExecutedTasks;
        returnedTime = CurrentRodosTime() + id * h;
        return returnedTime;
    }
};


class NeverDueTask
{
public:
    static constexpr auto startTime = endOfTime;
    int nExecutions = 0;

    auto Execute() -> RodosTime
    {
        ++nExecutions;
        return endOfTime;
    }
};


class TaskWithInitialize
{
public:
    static constexpr auto startTime = endOfTime;
    bool isInitialized = false;

    auto Initialize() -> void
    {
        isInitialized = true;
    }

    auto Execute() -> RodosTime
    {
        return endOfTime;
    }
};


class TaskReturningThePast
{
public:
    static constexpr auto startTime = RodosTime(0);

    auto Execute() -> RodosTime
    {
        return RodosTime(0);
    }
};


class TaskWithoutStartTime
{
public:
    auto Execute() -> RodosTime
    {
        return endOfTime;
    }
};


class TaskWithoutExecute
{
public:
    static constexpr auto startTime = RodosTime(0);
};


class TaskWithWrongReturnType
{
public:
    static constexpr auto startTime = RodosTime(0);

    auto Execute() -> void
    {}
};


class TaskWithoutDefaultConstructor
{
public:
    static constexpr auto startTime = RodosTime(0);

    TaskWithoutDefaultConstructor() = delete;

    auto Execute() -> RodosTime
    {
        return endOfTime;
    }
};
}


static_assert(LightweightTask<DueTask<1>>);
static_assert(LightweightTask<NeverDueTask>);
static_assert(LightweightTask<TaskWithInitialize>);
static_assert(not LightweightTask<TaskWithoutStartTime>);
static_assert(not LightweightTask<TaskWithoutExecute>);
static_assert(not LightweightTask<TaskWithWrongReturnType>);
static_assert(not LightweightTask<TaskWithoutDefaultConstructor>);


TEST_CASE("Initialize() is only called for tasks that have it")
{
    auto scheduler = Scheduler<TaskWithInitialize, NeverDueTask>{};
    CHECK(scheduler.Get<TaskWithInitialize>().isInitialized == false);

    scheduler.Initialize();
    CHECK(scheduler.Get<TaskWithInitialize>().isInitialized);
}


TEST_CASE("Only due tasks are executed")
{
    auto scheduler = Scheduler<DueTask<1>, NeverDueTask>{};
    (void)scheduler.ExecuteDueTasks();
    CHECK(scheduler.Get<DueTask<1>>().nExecutions == 1);
    CHECK(scheduler.Get<NeverDueTask>().nExecutions == 0);

    // DueTask<1> wants to run again in 1h, this is why it must not be executed again now
    (void)scheduler.ExecuteDueTasks();
    CHECK(scheduler.Get<DueTask<1>>().nExecutions == 1);
}


TEST_CASE("Tasks are executed in the order of the template arguments")
{
    nExecutedTasks = 0;
    auto scheduler = Scheduler<DueTask<2>, DueTask<1>>{};
    (void)scheduler.ExecuteDueTasks();
    CHECK(scheduler.Get<DueTask<2>>().executionNumber == 1);
    CHECK(scheduler.Get<DueTask<1>>().executionNumber == 2);
}


TEST_CASE("ExecuteDueTasks() returns the earliest next execution time")
{
    auto scheduler = Scheduler<DueTask<2>, DueTask<1>, NeverDueTask>{};
    auto nextExecutionTime = scheduler.ExecuteDueTasks();
    CHECK(nextExecutionTime == scheduler.Get<DueTask<1>>().returnedTime);
}


TEST_CASE("ExecuteDueTasks() returns endOfTime if no task will ever be due")
{
    auto scheduler = Scheduler<NeverDueTask>{};
    auto nextExecutionTime = scheduler.ExecuteDueTasks();
    CHECK(nextExecutionTime == endOfTime);
}


TEST_CASE("Tasks returning a time in the past are delayed by minimumTaskInterval")
{
    auto scheduler = Scheduler<TaskReturningThePast>{};
    auto timeBeforeExecution = CurrentRodosTime();
    auto nextExecutionTime = scheduler.ExecuteDueTasks();
    CHECK(nextExecutionTime >= timeBeforeExecution + minimumTaskInterval);
}
}
