#include <Sts1CobcSw/CobcCommands.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/PersistentState.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
auto eduEnabledGpio = hal::GpioPin(hal::eduEnabledPin);


void TurnEduOn()
{
    RODOS::PRINTF("*Turn on EDU*\n");
    // The enable pin uses inverted logic: 0 = enable
    eduEnabledGpio.Reset();
    // Set EduShouldBePowered to True, persistentstate is initialized in EduPowerManagementThread.cpp
    periphery::persistentstate::EduShouldBePowered(true);

}


void TurnEduOff()
{
    RODOS::PRINTF("*Turn off EDU*\n");
    // The enable pin uses inverted logic: 1 = disable
    eduEnabledGpio.Set();
    // Set EduShouldBePowered to False, persistentstate is initialized in EduPowerManagementThread.cpp
    periphery::persistentstate::EduShouldBePowered(false);
}
}
