#include <Sts1CobcSw/CobcCommands.hpp>
#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/Topics.hpp>

#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Util/Util.hpp>
#include <type_safe/index.hpp>
#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/map.h>
#include <etl/string.h>
#include <etl/string_view.h>

#include <cstring>


namespace RODOS
{
#if defined(LINUX_SYSTEM)
// NOLINTNEXTLINE(readability-identifier-naming)
HAL_UART uart_stdout(RODOS::UART_IDX2);
#elif defined(GENERIC_SYSTEM)
// NOLINTNEXTLINE(readability-identifier-naming)
extern HAL_UART uart_stdout;
#endif
}

namespace sts1cobcsw
{
auto DispatchCommand(const etl::string<commandSize.get()> & command)
{
    auto targetIsCobc = command[1] == '0';
    auto commandId = command[2];

    if(targetIsCobc)
    {
        switch(commandId)
        {
            case '1':
            {
                TurnEduOn();
                return;
            }
            case '2':
            {
                TurnEduOff();
                return;
            }
            case '3':
            {
                UpdateUtcOffset();
                return;
            }
            default:
            {
                break;
            }
        }
    }

    RODOS::PRINTF("*Error, invalid command*\n");
}


class CommandParserThread : public RODOS::StaticThread<>
{
    void init() override
    {
        eduEnabledGpio.init(/*isOutput=*/true, 1, 0);
    }

    void run() override
    {
        constexpr auto startCharacter = '$';

        auto command = etl::string<commandSize.get()>();
        ts::bool_t startWasDetected = false;
        while(true)
        {
            char readCharacter = 0;
            auto nReadCharacters = ts::size_t(RODOS::uart_stdout.read(&readCharacter, 1));
            if(nReadCharacters != 0U)
            {
                RODOS::PRINTF("Read a charactere\n");
                if(readCharacter == startCharacter)
                {
                    startWasDetected = true;
                    command.clear();
                    command += startCharacter;
                }
                else if(startWasDetected)
                {
                    command += readCharacter;
                    if(command.full())
                    {
                        DispatchCommand(command);
                        startWasDetected = false;
                    }
                }
            }
            RODOS::uart_stdout.suspendUntilDataReady();
        }
    }
} commandParserThread;


class GsFaker : public RODOS::StaticThread<>
{
    void run() override
    {
        constexpr auto temperatureCommand = "$51\n";

        RODOS::AT(RODOS::NOW() + 2 * RODOS::SECONDS);
        RODOS::PRINTF("Writing to uart_stdout");
        hal::WriteTo(&RODOS::uart_stdout, temperatureCommand);
    }
} gsFaker;
}
