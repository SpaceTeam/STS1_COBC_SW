#include <Sts1CobcSw/CobcCommands.hpp>
#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>

#include <rodos_no_using_namespace.h>

// TODO: why is this here ?
//#include <stm32f4xx_rtc.h>

using RODOS::PRINTF;

namespace sts1cobcsw
{
void TurnEduOn()
{
    PRINTF("*Turn on EDU*\n");
    // The enable pin uses inverted logic: 0 = enable
    eduEnabledGpio.setPins(0U);
    // eduEnabledGpio.reset();
}


void TurnEduOff()
{
    PRINTF("*Turn off EDU*\n");
    // The enable pin uses inverted logic: 1 = disable
    eduEnabledGpio.setPins(1U);
    // TODO:
    // eduEnabledGpio.set();
}

void UpdateUtcOffset()
{
    PRINTF("Update UTC offset");
}


static TimeEvent te01;

void BuildQueue()
{
    // TODO: The actual parsing is done in command parser, call this function at the end or move
    // this time event handle there
    // When the queue is parsed, we can resume the EduProgramQueueThread
    te01.handle();
}


}
