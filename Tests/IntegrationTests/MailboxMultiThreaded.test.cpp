#include <Sts1CobcSw/Mailbox/Mailbox.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdlib>
#include <utility>


namespace sts1cobcsw
{
namespace
{
auto mailbox = Mailbox<int>{};
auto errorCounter = 0;


class Sender : public RODOS::StaticThread<>
{
    void run() override
    {
        for(auto counter = 0; counter < 2; ++counter)
        {
            auto result = mailbox.Put(counter);
            if(result.has_error())
            {
                errorCounter++;
            }
            result = mailbox.SuspendUntilEmpty(15 * ms);
            if(result.has_error())
            {
                errorCounter++;
            }
        }
        SuspendFor(10 * ms);
        if(errorCounter == 0)
        {
            RODOS::PRINTF("Test passed\n");
        }
        else
        {
            RODOS::PRINTF("Test failed with %d errors\n", errorCounter);
        }
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
            auto value = mailbox.Get();
            if(value.has_error() or value.value() != counter)
            {
                errorCounter++;
            }
            counter++;
            auto result = mailbox.SuspendUntilFull(15 * ms);
            if(result.has_error())
            {
                errorCounter++;
            }
        }
    }
} receiver;
}
}
