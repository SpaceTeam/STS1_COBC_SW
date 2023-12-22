#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/Enums.hpp>
#include <Sts1CobcSw/Edu/Structs.hpp>
#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <rodos_no_using_namespace.h>

#include <charconv>
#include <cinttypes>
#include <cstdint>
#include <span>


namespace sts1cobcsw
{
using RODOS::PRINTF;


auto uciUart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);


template<std::size_t nCharacters>
auto ReadCharacters() -> std::array<char, nCharacters>;


class EduCommandsTest : public RODOS::StaticThread<>
{
public:
    EduCommandsTest() : StaticThread("EduCommandsTest")
    {
    }

private:
    void init() override
    {
        edu::Initialize();
        auto const baudRate = 115'200;
        hal::Initialize(&uciUart, baudRate);
    }


    void run() override
    {
        // Permanently turn on EDU for this test
        edu::TurnOn();

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

            auto command = ReadCharacters<1>()[0];
            PRINTF("\n");
            switch(command)
            {
                case 'u':
                {
                    auto currentTime = utility::GetUnixUtc();
                    PRINTF("Sending UpdateTime(currentTime = %d)\n", static_cast<int>(currentTime));
                    auto updateTimeResult = edu::UpdateTime({.currentTime = currentTime});
                    if(updateTimeResult.has_error())
                    {
                        PRINTF("Returned error code: %d\n",
                               static_cast<int>(updateTimeResult.error()));
                    }
                    else
                    {
                        PRINTF("UpdateTime executed successfully\n");
                    }
                    break;
                }
                case 'e':
                {
                    PRINTF("Please enter a program ID (1 character)\n");
                    auto userInput = ReadCharacters<1>();
                    std::uint16_t programId = 0;
                    std::from_chars(begin(userInput), end(userInput), programId);

                    PRINTF("Please enter a start time (1 character)\n");
                    userInput = ReadCharacters<1>();
                    std::int32_t startTime = 0;
                    std::from_chars(begin(userInput), end(userInput), startTime);

                    PRINTF("Please enter a timeout (1 character)\n");
                    userInput = ReadCharacters<1>();
                    std::int16_t timeout = 0;
                    std::from_chars(begin(userInput), end(userInput), timeout);

                    PRINTF("\n");
                    PRINTF("Sending ExecuteProgram(programId = %" PRIu16 ", startTime = %" PRIi32
                           ", timeout = %" PRIi16 ")\n",
                           programId,
                           startTime,
                           timeout);
                    auto executeProgramResult = edu::ExecuteProgram(
                        {.programId = programId, .startTime = startTime, .timeout = timeout});
                    if(executeProgramResult.has_error())
                    {
                        PRINTF("Returned error code: %d\n",
                               static_cast<int>(executeProgramResult.error()));
                    }
                    else
                    {
                        PRINTF("Execute Program Returned no error");
                    }
                    break;
                }
                case 'g':
                {
                    PRINTF("Sending GetStatus()\n");
                    auto getStatusResult = edu::GetStatus();
                    PRINTF("Returned status:\n");
                    if(getStatusResult.has_error())
                    {
                        PRINTF("  error code = %d\n", static_cast<int>(getStatusResult.error()));
                    }
                    else
                    {
                        auto status = getStatusResult.value();
                        PRINTF("  type       = %d\n", static_cast<int>(status.statusType));
                        PRINTF("  program ID = %d\n", static_cast<int>(status.programId));
                        PRINTF("  startTime  = %d\n", static_cast<int>(status.startTime));
                        PRINTF("  exit code  = %d\n", static_cast<int>(status.exitCode));
                    }
                    break;
                }
                case 'r':
                {
                    PRINTF("Sending ReturnResult()\n");
                    auto returnResultResult = edu::ReturnResult();
                    PRINTF("Returned result info:\n");
                    if(returnResultResult.has_error())
                    {
                        PRINTF("  error code  = %d\n",
                               static_cast<int>(returnResultResult.error()));
                    }
                    else
                    {
                        PRINTF("  result size = %d\n",
                               static_cast<int>(returnResultResult.value().resultSize));
                    }
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


template<std::size_t nCharacters>
auto ReadCharacters() -> std::array<char, nCharacters>
{
    auto string = std::array<char, nCharacters>{};
    // NOLINTNEXTLINE(*reinterpret-cast)
    hal::ReadFrom(&uciUart, std::span(reinterpret_cast<Byte *>(string.data()), nCharacters));
    return string;
}
}
