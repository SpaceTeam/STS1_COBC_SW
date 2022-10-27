#include <Sts1CobcSw/CobcCommands.hpp>
#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Topics.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <type_safe/index.hpp>
#include <type_safe/narrow_cast.hpp>
#include <type_safe/types.hpp>

#include <rodos-assert.h>
#include <timemodel.h>

#include <rodos_no_using_namespace.h>

#include <cstdint>
#include <span>


namespace RODOS
{
// NOLINTNEXTLINE
extern HAL_UART uart_stdout;
}

namespace sts1cobcsw
{
namespace ts = type_safe;
using ts::operator""_usize;


constexpr auto queueEntrySize = 10;

enum CommandType : char
{
    turnEduOn = '1',
    turnEduOff = '2',
    buildQueue = '4'
};


// TODO: Use Deserialize instead of CopyFrom
auto ParseQueueEntries(const etl::string<commandSize.get()> & command)
{
    auto const nQueueEntries = command.size() / queueEntrySize;
    eduProgramQueue.resize(nQueueEntries);

    ts::size_t position = 0U;

    for(auto & entry : eduProgramQueue)
    {
        uint16_t progId = 0;
        CopyFrom(command, &position, &progId);
        RODOS::PRINTF("Prog ID      : %ld\n", progId);  // NOLINT
        uint16_t queueId = 0;
        CopyFrom(command, &position, &queueId);
        RODOS::PRINTF("Queue ID     : %ld\n", queueId);  // NOLINT
        int32_t startTime = 0;
        CopyFrom(command, &position, &startTime);
        RODOS::PRINTF("Start Time   : %ld\n", startTime);  // NOLINT
        int16_t timeout = 0;
        CopyFrom(command, &position, &timeout);
        RODOS::PRINTF("Timeout      : %ld\n", timeout);  // NOLINT

        entry = QueueEntry{
            .programId = progId, .queueId = queueId, .startTime = startTime, .timeout = timeout};
    }
}

auto DispatchCommand(const etl::string<commandSize.get()> & command)
{
    auto targetIsCobc = true;
    ts::size_t position = 1_usize;
    int32_t utc = 0;
    char commandId = 0;
    int16_t length = 0;

    CopyFrom(command, &position, &utc);
    RODOS::sysTime.setUTC(utility::UnixToRodosTime(utc));
    utility::PrintTime();

    CopyFrom(command, &position, &commandId);
    RODOS::PRINTF("command ID is character : %c\n", commandId);

    CopyFrom(command, &position, &length);
    RODOS::PRINTF("Length of data is : %d\n", length);

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
            case CommandType::buildQueue:
            {
                RODOS::PRINTF("Entering build queue command parsing\n");

                auto const nbQueueEntries = length / queueEntrySize;
                RODOS::PRINTF("Number of queue entries : %d\n", nbQueueEntries);

                // Erase all previous entries on program queue
                EmptyEduProgramQueue();

                ParseQueueEntries(command.substr(position.get(), length));

                // Reset queue index and resume EduProgramQueueThread
                ResetQueueIndex();
                ResumeEduQueueThread();

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
        eduEnabledGpio.Direction(hal::PinDirection::out);
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
