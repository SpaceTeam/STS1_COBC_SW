#include <Sts1CobcSw/CobcCommands.hpp>
#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>

#include <rodos_no_using_namespace.h>

// TODO why is this here ?
//#include <stm32f4xx_rtc.h>

using RODOS::PRINTF;

namespace sts1cobcsw
{
void TurnEduOn()
{
    PRINTF("*Turn on EDU*\n");
    // The enable pin uses inverted logic: 0 = enable
    eduEnabledGpio.setPins(0U);
}


void TurnEduOff()
{
    PRINTF("*Turn off EDU*\n");
    // The enable pin uses inverted logic: 1 = disable
    eduEnabledGpio.setPins(1U);
}

void UpdateUtcOffset()
{
    PRINTF("Update UTC offset");
    // TODO
}

static TimeEventTest te01;

void BuildQueue() {

    PRINTF("Build a program queue");

    // TODO actual parsing of the queue
        
    // When the queue is parsed, we can resume the EduProgramQueueThread
    te01.handle();
}


}
