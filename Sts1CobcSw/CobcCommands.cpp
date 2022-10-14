#include <Sts1CobcSw/CobcCommands.hpp>

#include <Sts1CobcSw/CommandParser.hpp>


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
    // Set EduShouldBePowered auf True
    hal::SetPin(eduEnableGpio, false);
}


void TurnEduOff()
{
    PRINTF("*Turn off EDU*\n");
    // The enable pin uses inverted logic: 1 = disable
    // Set EduShouldBePowered auf False
    hal::SetPin(eduEnableGpio, true);
}
}
