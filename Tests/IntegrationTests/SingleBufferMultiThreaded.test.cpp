#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/SingleBuffer/SingleBuffer.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
auto buffer = SingleBuffer<int>{};
auto errorCounter = 0;


class Sender : public RODOS::StaticThread<>
{
    void run() override
    {
        for(auto counter = 0; counter < 2; ++counter)
        {
            auto result = buffer.Put(counter);
            if(result.has_error())
            {
                errorCounter++;
            }
            result = buffer.SuspendUntilEmpty(15 * ms);
            if(result.has_error())
            {
                errorCounter++;
            }
        }
        SuspendFor(10 * ms);
        RODOS::isShuttingDown = true;
        std::exit(errorCounter);  // NOLINT(concurrency-mt-unsafe)
    }
} sender;


class Receiver : public RODOS::StaticThread<>
{
    void run() override
    {
        auto counter = 0;
        SuspendFor(10 * ms);
        while(true)
        {
            auto value = buffer.Get();
            if(value.has_error())
            {
                errorCounter++;
            }
            if(value.has_value())
            {
                if(value.value() != counter)
                {
                    errorCounter++;
                }
            }
            counter++;
            auto result = buffer.SuspendUntilFull(15 * ms);
            if(result.has_error())
            {
                errorCounter++;
            }
        }
    }
} receiver;
}
