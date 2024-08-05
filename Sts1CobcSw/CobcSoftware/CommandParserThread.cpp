#include <Sts1CobcSw/CobcSoftware/CommandParser.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>

#include <cstddef>


namespace RODOS
{
// NOLINTNEXTLINE
extern HAL_UART uart_stdout;
}


namespace sts1cobcsw
{
// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 4'000U;
constexpr auto startCharacter = '$';


class CommandParserThread : public RODOS::StaticThread<stackSize>
{
public:
    CommandParserThread() : StaticThread("CommandParserThread", commandParserThreadPriority)
    {
    }

private:
    void init() override
    {
    }


    void run() override
    {
        auto command = etl::vector<Byte, commandSize>();
        auto startWasDetected = false;

        while(true)
        {
            char readCharacter = 0;
            // TODO: This needs to be abstracted away because "IRL" we receive commands via the RF
            // module
            std::size_t const nReadCharacters =
                RODOS::uart_stdout.read(&readCharacter, sizeof(readCharacter));
            if(nReadCharacters == 0)
            {
                RODOS::uart_stdout.suspendUntilDataReady();
                continue;
            }

            if(readCharacter == startCharacter)
            {
                startWasDetected = true;
                command.clear();
            }
            if(startWasDetected)
            {
                command.push_back(static_cast<Byte>(readCharacter));
                // Every command has the same size
                if(command.full())
                {
                    DispatchCommand(command);
                    startWasDetected = false;
                }
            }
        }
    }
} commandParserThread;
}
