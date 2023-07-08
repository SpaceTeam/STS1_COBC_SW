#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/ThreadPriorities.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <type_safe/index.hpp>
#include <type_safe/narrow_cast.hpp>
#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>

#include <cstdint>
#include <cstring>
#include <span>


namespace RODOS
{
// NOLINTNEXTLINE
extern HAL_UART uart_stdout;
}

namespace sts1cobcsw
{
namespace ts = type_safe;

using sts1cobcsw::serial::Byte;
using sts1cobcsw::serial::SerialBuffer;

// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 4'000U;

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
        constexpr auto startCharacter = '$';

        // TODO: The command is not a string.
        auto command = etl::vector<Byte, commandSize>();
        ts::bool_t startWasDetected = false;

        while(true)
        {
            char readCharacter = 0;
            // TODO: This needs to be abstracted away because "IRL" we receive commands via the RF
            // module
            ts::size_t nReadCharacters =
                RODOS::uart_stdout.read(&readCharacter, sizeof(readCharacter));
            if(nReadCharacters != 0U)
            {
                // RODOS::PRINTF("Read a character : %c\n", readCharacter);
                if(readCharacter == startCharacter)
                {
                    startWasDetected = true;
                    command.clear();
                    command.push_back(static_cast<Byte>(startCharacter));
                }
                else if(startWasDetected)
                {
                    command.push_back(static_cast<Byte>(startCharacter));
                    // Every command has the same size
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
}
