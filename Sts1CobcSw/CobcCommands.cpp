#include <Sts1CobcSw/CobcCommands.hpp>
#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
void TurnEduOn()
{
    RODOS::PRINTF("*Turn on EDU*\n");
    // The enable pin use inverted logic: 0 = enable
    eduEnabledGpio.Reset();
}


void TurnEduOff()
{
    RODOS::PRINTF("*Turn off EDU*\n");
    // The enable pin uses inverted logic: 1 = disable
    eduEnabledGpio.Set();
}

void UpdateUtcOffset()
{
    RODOS::PRINTF("Update UTC offset");
}


static TimeEvent eduProgramQueueThreadTimeEvent;

void BuildQueue()
{
    eduProgramQueueThreadTimeEvent.handle();
}


}
