#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>

#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Uart.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using RODOS::PRINTF;


hal::GpioPin eduUpdateGpioPin(hal::eduUpdatePin);
auto uciUart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);

constexpr auto stackSize = 2'000U;


class EduPowerManagementTest : public RODOS::StaticThread<stackSize>
{
public:
    EduPowerManagementTest() : StaticThread("EduPowerManagementTest")
    {
    }

private:
    void init() override
    {
#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Direction(hal::PinDirection::out);
#endif
        edu::Initialize();
        eduUpdateGpioPin.Direction(hal::PinDirection::in);
        auto const baudRate = 115'200;
        hal::Initialize(&uciUart, baudRate);
    }


    void run() override
    {
#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Reset();
#endif

        PRINTF("\n");
        PRINTF("EDU power management test\n");

        // std::int64_t startDelay = 0;

        /*
        while(true)
        {
            nextProgramStartDelayBuffer.get(startDelay);
            PRINTF("\n");
            PRINTF("startDelay = %d s\n", static_cast<int>(startDelay / RODOS::SECONDS));

            PRINTF("\n");
            PRINTF("What do you want to do?\n");
            PRINTF("  s: set startDelay\n");

            auto command = std::array<char, 1>{};
            hal::ReadFrom(&uciUart, std::span(command));
            switch(command[0])
            {
                case 's':
                {
                    auto userInput = std::array<char, 2>{};
                    PRINTF("\n");
                    PRINTF("Please enter the start delay in seconds (2 characters)\n");
                    hal::ReadFrom(&uciUart, std::span(userInput));
                    std::from_chars(begin(userInput), end(userInput), startDelay);
                    startDelay *= RODOS::SECONDS;
                    nextProgramStartDelayTopic.publish(startDelay);
                    break;
                }
                default:
                {
                    PRINTF("\n");
                    PRINTF("Unknown command\n");
                    break;
                }
            }
        }

        */
    }
} eduPowerManagementTest;
}
