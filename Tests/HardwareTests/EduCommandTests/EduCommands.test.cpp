#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduEnums.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <rodos_no_using_namespace.h>

#include <charconv>
#include <cstdint>


namespace sts1cobcsw
{
using RODOS::PRINTF;
using sts1cobcsw::serial::Byte;


auto edu = periphery::Edu();
auto uciUart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);


class EduCommandsTest : public RODOS::StaticThread<>
{
public:
    EduCommandsTest() : StaticThread("EduCommandsTest")
    {
    }

private:
    void init() override
    {
        edu.Initialize();
        uciUart.init();
    }


    void run() override
    {
        // Permanently turn on EDU for this test
        edu.TurnOn();

        PRINTF("\n");
        PRINTF("EDU commands test\n");

        while(true)
        {
            PRINTF("\n");
            PRINTF("Which command do you want to send to the EDU?\n");
            PRINTF("  u: UpdateTime\n");
            PRINTF("  e: ExecuteProgram\n");
            PRINTF("  g: GetStatus\n");
            PRINTF("  r: ReturnResult\n");

            auto command = std::array<char, 1>{};
            hal::ReadFrom(&uciUart, std::span(command));
            PRINTF("\n");
            switch(command[0])
            {
                case 'u':
                {
                    auto timestamp = utility::GetUnixUtc();
                    PRINTF("UpdateTime(timestamp = %d)\n", static_cast<int>(timestamp));
                    auto errorCode = edu.UpdateTime({.timestamp = timestamp});
                    PRINTF("  error code: %d\n", static_cast<int>(errorCode));
                    break;
                }
                case 'e':
                {
                    auto userInput = std::array<char, 3>{};

                    PRINTF("Please enter a program ID (3 character)\n");
                    hal::ReadFrom(&uciUart, std::span(userInput));
                    std::uint16_t programId = 0;
                    std::from_chars(begin(userInput), end(userInput), programId);

                    PRINTF("Please enter a queue ID (3 character)\n");
                    hal::ReadFrom(&uciUart, std::span(userInput));
                    std::uint32_t queueId = 0;
                    std::from_chars(begin(userInput), end(userInput), queueId);

                    PRINTF("Please enter a timeout (3 character)\n");
                    hal::ReadFrom(&uciUart, std::span(userInput));
                    std::int16_t timeout = 0;
                    std::from_chars(begin(userInput), end(userInput), timeout);

                    PRINTF("\n");
                    auto errorCode = edu.ExecuteProgram(
                        {.programId = programId, .queueId = queueId, .timeout = timeout});
                    PRINTF("  error code = %d\n", static_cast<int>(errorCode));
                    break;
                }
                case 'g':
                {
                    [[maybe_unused]] auto status = edu.GetStatus();
                    break;
                }
                case 'r':
                {
                    [[maybe_unused]] auto resultInfo = edu.ReturnResult();
                    break;
                }
                default:
                {
                    PRINTF("Unknown command\n");
                    break;
                }
            }
        }
    }
} eduCommandsTest;
}
