#include <Sts1CobcSw/Utility.hpp>

#include <Sts1CobcSw/Hal/Gpio.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>
#include <type_safe/narrow_cast.hpp>
#include <type_safe/types.hpp>

#include <rodos.h>


namespace sts1cobcsw
{
namespace ts = type_safe;
using ts::operator""_usize;

// TODO : Remove this when hal is merged
constexpr auto pb1 = RODOS::GPIO_017;
constexpr auto eduUpdatePin = pb1;
auto eduUpdateGpio = HAL_GPIO(eduUpdatePin);

class EduListenerThread : public StaticThread<>
{
    void init() override
    {
        // TODO uncomment next line and remove the following when hal is merged
        // hal::InitPin(eduUpdateGpio, hal::PinType::input, hal::PinVal::one);
        eduUpdateGpio.init(/*isOutput=*/false, 1U, 0U);
    }

    void run() override
    {
        auto const eduHasUpdate = eduUpdateGpio.readPins() != 0;

        if(eduHasUpdate)
        {
            // Communicate with EDU

            // TODO this will be implemented by Daniel, will look something like
            // auto status = periphery::edu::GetStatus();

            // Possible outputs are : 
            //      - No event
            //      - Program Finished : Program-ID, Queue-ID and exit code are provided
            //      - Results Ready : Program-ID, Queue-ID are provided
            
            auto programHasFinished = true;
            auto event = true;
            if(programHasFinished) {
                // Update newest Status_and_History
                // Resume EDU Queue thread
            } else if(event) {

                // What need get result file
                // This is done with the get result command

                // Interpret the result file and update the resulting SaHEntry from 2 or 3 to 4.
            }
        }

        AT(NOW() + 1 * SECONDS);
    }
};

auto const eduListenerThread = EduListenerThread();
}
