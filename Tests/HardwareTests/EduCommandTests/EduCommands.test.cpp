#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/Periphery/Enums.hpp>
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
                    PRINTF("Sending UpdateTime(timestamp = %d)\n", static_cast<int>(timestamp));
                    auto errorCode = edu.UpdateTime({.timestamp = timestamp});
                    PRINTF("Returned error code: %d\n", static_cast<int>(errorCode));
                    break;
                }
                case 'e':
                {
                    auto userInput = std::array<char, 1>{};

                    PRINTF("Please enter a program ID (1 character)\n");
                    hal::ReadFrom(&uciUart, std::span(userInput));
                    std::uint16_t programId = 0;
                    std::from_chars(begin(userInput), end(userInput), programId);

                    PRINTF("Please enter a queue ID (1 character)\n");
                    hal::ReadFrom(&uciUart, std::span(userInput));
                    std::uint16_t queueId = 0;
                    std::from_chars(begin(userInput), end(userInput), queueId);

                    PRINTF("Please enter a timeout (1 character)\n");
                    hal::ReadFrom(&uciUart, std::span(userInput));
                    std::int16_t timeout = 0;
                    std::from_chars(begin(userInput), end(userInput), timeout);

                    PRINTF("\n");
                    PRINTF("Sending ExecuteProgram(programId = %d, queueId = %d, timeout = %d)\n",
                           static_cast<int>(programId),
                           static_cast<int>(queueId),
                           static_cast<int>(timeout));
                    auto errorCode = edu.ExecuteProgram(
                        {.programId = programId, .queueId = queueId, .timeout = timeout});
                    PRINTF("Returned error code: %d\n", static_cast<int>(errorCode));
                    break;
                }
                case 'g':
                {
                    PRINTF("Sending GetStatus()\n");
                    auto status = edu.GetStatus();
                    PRINTF("Returned status:\n");
                    PRINTF("  type       = %d\n", static_cast<int>(status.statusType));
                    PRINTF("  program ID = %d\n", static_cast<int>(status.programId));
                    PRINTF("  queue ID   = %d\n", static_cast<int>(status.queueId));
                    PRINTF("  exit code  = %d\n", static_cast<int>(status.exitCode));
                    PRINTF("  error code = %d\n", static_cast<int>(status.errorCode));
                    break;
                }
                case 'r':
                {
                    PRINTF("Sending ReturnResult()\n");
                    auto resultInfo = edu.ReturnResult();
                    PRINTF("Returned result info:\n");
                    PRINTF("  error code  = %d\n", static_cast<int>(resultInfo.errorCode));
                    PRINTF("  result size = %d\n", static_cast<int>(resultInfo.resultSize.get()));
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
} executeProgramTest;
}
