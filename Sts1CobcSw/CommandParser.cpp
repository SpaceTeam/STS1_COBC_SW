#include <Sts1CobcSw/CobcCommands.hpp>
#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Topics.hpp>
#include <Sts1CobcSw/Util/Time.hpp>
#include <Sts1CobcSw/Util/Util.hpp>

#include <type_safe/index.hpp>
#include <type_safe/narrow_cast.hpp>
#include <type_safe/types.hpp>

#include <rodos-assert.h>
#include <timemodel.h>

#include <rodos_no_using_namespace.h>

#include <etl/map.h>
#include <etl/string.h>
#include <etl/string_view.h>

#include <cinttypes>
#include <cstring>
#include <span>
#include <tuple>


namespace RODOS
{
#if defined(LINUX_SYSTEM)
// TODO: remove this
// NOLINTNEXTLINE(readability-identifier-naming)
HAL_UART uart_stdout(RODOS::UART_IDX2);
#elif defined(GENERIC_SYSTEM)
// NOLINTNEXTLINE(readability-identifier-naming)
extern HAL_UART uart_stdout;
#endif
}

namespace sts1cobcsw
{
namespace ts = type_safe;
using ts::operator""_usize;


enum CommandType
{
    turnEduOn = '1',
    turnEduOff = '2',
    updateUtcOffset = '3',
    buildQueue = '4'
};

auto DispatchCommand(const etl::string<commandSize.get()> & command)
{
    auto targetIsCobc = true;
    ts::size_t position = 1_usize;
    int32_t utc = 0;
    char commandId = 0;

    util::CopyFrom(command, &position, &utc);
    RODOS::sysTime.setUTC(util::UnixToRodosTime(utc));
    util::PrintTime();

    util::CopyFrom(command, &position, &commandId);
    RODOS::PRINTF("command ID is character : %c\n", commandId);

    if(targetIsCobc)
    {
        switch(commandId)
        {
            case CommandType::turnEduOn:
            {
                TurnEduOn();
                return;
            }
            case CommandType::turnEduOff:
            {
                TurnEduOff();
                return;
            }
            case CommandType::updateUtcOffset:
            {
                UpdateUtcOffset();
                return;
            }
            case CommandType::buildQueue:
            {
                RODOS::PRINTF("Entering build queue command parsing\n");
                uint16_t length = 0;
                util::CopyFrom(command, &position, &length);
                RODOS::PRINTF("Length of data is : %d\n", length);

                if(length % queueEntrySize != 0)
                {
                    return;
                }

                auto const nbQueueEntries = length / queueEntrySize;
                RODOS::PRINTF("Number of queue entries : %d\n", nbQueueEntries);

                EmptyEduProgramQueue();

                for(auto i = 0; i < nbQueueEntries; ++i)
                {
                    uint16_t progId = 0;
                    util::CopyFrom(command, &position, &progId);
                    uint16_t queueId = 0;
                    RODOS::PRINTF("Prog ID      : %ld\n", progId);  // NOLINT
                    util::CopyFrom(command, &position, &queueId);
                    RODOS::PRINTF("Queue ID     : %ld\n", queueId);  // NOLINT
                    uint32_t startTime = 0;
                    util::CopyFrom(command, &position, &startTime);
                    RODOS::PRINTF("Start Time   : %ld\n", startTime);  // NOLINT
                    uint16_t timeout = 0;
                    util::CopyFrom(command, &position, &timeout);
                    RODOS::PRINTF("Timeout      : %ld\n", timeout);  // NOLINT

                    auto queueEntry = QueueEntry{.programId = progId,
                                                 .queueId = queueId,
                                                 .startTime = startTime,
                                                 .timeout = timeout};
                    AddQueueEntry(queueEntry);
                }
                ResetQueueIndex();
                BuildQueue();

                return;
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
                // RODOS::PRINTF("Read a character : %c\n", readCharacter);
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

template<std::size_t size>
auto ComputeChecksum(std::span<std::byte, size> data)
{
    static_assert(size >= 3,
                  "The size of 'beacon' must be >= 3 because the start, stop and "
                  "checksum bytes are not included in the computation.");
    // TODO: fix this mess with type safe
    return static_cast<uint8_t>(std::accumulate(std::begin(data) + 1,
                                                std::end(data) - 2,
                                                0,
                                                [](auto sum, auto currentElement)
                                                {
                                                    auto x = static_cast<uint8_t>(currentElement);
                                                    return sum + x;
                                                }));
}
}
