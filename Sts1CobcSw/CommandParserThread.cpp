#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <type_safe/index.hpp>
#include <type_safe/narrow_cast.hpp>
#include <type_safe/types.hpp>

#include <rodos-assert.h>
#include <timemodel.h>

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
using sts1cobcsw::serial::DeserializeFrom;


enum CommandId : char
{
    turnEduOn = '1',
    turnEduOff = '2',
    buildQueue = '4',
};

struct GsCommandHeader
{
    char startCharacter;
    std::int32_t utc;
    std::int8_t commandId;
    std::int16_t length;
};
constexpr auto gsCommandHeaderSize = sizeof(GsCommandHeader);


namespace serial
{
template<>
inline constexpr std::size_t serialSize<GsCommandHeader> =
    totalSerialSize<decltype(GsCommandHeader::startCharacter),
                    decltype(GsCommandHeader::utc),
                    decltype(GsCommandHeader::commandId),
                    decltype(GsCommandHeader::length)>;

template<>
inline constexpr std::size_t serialSize<EduQueueEntry> =
    totalSerialSize<decltype(EduQueueEntry::programId),
                    decltype(EduQueueEntry::queueId),
                    decltype(EduQueueEntry::startTime),
                    decltype(EduQueueEntry::timeout)>;
}


auto DeserializeFrom(Byte * source, GsCommandHeader * data) -> Byte *
{
    source = DeserializeFrom(source, &(data->startCharacter));
    source = DeserializeFrom(source, &(data->utc));
    source = DeserializeFrom(source, &(data->commandId));
    source = DeserializeFrom(source, &(data->length));
    return source;
}


auto DeserializeFrom(Byte * source, EduQueueEntry * data) -> Byte *
{
    source = DeserializeFrom(source, &(data->queueId));
    source = DeserializeFrom(source, &(data->programId));
    source = DeserializeFrom(source, &(data->startTime));
    source = DeserializeFrom(source, &(data->startTime));
    return source;
}

// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 4'000U;
constexpr std::size_t commandSize = 30;
constexpr auto threadPriority = 100;
constexpr std::size_t queueEntrySize =
    sizeof(EduQueueEntry::programId) + sizeof(EduQueueEntry::queueId)
    + sizeof(EduQueueEntry::startTime) + sizeof(EduQueueEntry::timeout);


auto ParseAndAddQueueEntries(etl::string<commandSize> const & command) -> void;
auto DispatchCommand(etl::string<commandSize> const & command) -> void;


class CommandParserThread : public RODOS::StaticThread<stackSize>
{
public:
    CommandParserThread() : StaticThread("CommandParserThread", threadPriority)
    {
    }

private:
    void init() override
    {
    }


    void run() override
    {
        constexpr auto startCharacter = '$';

        auto command = etl::string<commandSize>();
        ts::bool_t startWasDetected = false;

        while(true)
        {
            char readCharacter = 0;
            // This needs to be abstracted away because "IRL" we receive commands via the RF module
            ts::size_t nReadCharacters =
                RODOS::uart_stdout.read(&readCharacter, sizeof(readCharacter));
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


auto DispatchCommand(etl::string<commandSize> const & command) -> void
{
    auto buffer = std::array<std::byte, serial::serialSize<GsCommandHeader>>();
    std::transform(
        std::begin(command), std::end(command), std::begin(buffer), [](char c) { return Byte(c); });

    // Debug print
    for(auto & b : buffer)
    {
        RODOS::PRINTF("%d", std::to_integer<uint8_t>(b));
    }

    auto gsCommandHeader = serial::Deserialize<GsCommandHeader>(buffer);

    // TODO: Why is this here? It has nothing to do with command dispatching
    RODOS::sysTime.setUTC(utility::UnixToRodosTime(gsCommandHeader.utc));

    utility::PrintFormattedSystemUtc();
    RODOS::PRINTF("Command ID character : %c\n", gsCommandHeader.commandId);
    RODOS::PRINTF("Length of data : %d\n", gsCommandHeader.length);

    auto targetIsCobc = true;
    if(targetIsCobc)
    {
        switch(gsCommandHeader.commandId)
        {
            case CommandId::turnEduOn:
            {
                edu.TurnOn();
                return;
            }
            case CommandId::turnEduOff:
            {
                edu.TurnOff();
                return;
            }
            case CommandId::buildQueue:
            {
                // TODO: This should be a function Build/BuildNew/Update/OverwriteEduQueue() or
                // something like that
                RODOS::PRINTF("Entering build queue command parsing\n");

                auto const nbQueueEntries =
                    gsCommandHeader.length / static_cast<int>(queueEntrySize);
                RODOS::PRINTF("Number of queue entries : %d\n", nbQueueEntries);

                // Erase all previous entries in the EDU program queue
                eduProgramQueue.clear();

                // TODO: use size of GsCommandHeader + 1
                ParseAndAddQueueEntries(command.substr(
                    gsCommandHeaderSize, static_cast<std::size_t>(gsCommandHeader.length)));

                // Reset queue index
                queueIndex = 0U;
                RODOS::PRINTF("Queue index reset. Current size of EDU program queue is %d.\n",
                              static_cast<int>(eduProgramQueue.size()));

                ResumeEduQueueThread();
                return;
            }
            default:
            {
                RODOS::PRINTF("*Error, invalid command*\n");
                return;
            }
        }
    }
}


// TODO: Use Deserialize instead of CopyFrom
auto ParseAndAddQueueEntries(etl::string<commandSize> const & command) -> void
{
    auto const nQueueEntries = command.size() / queueEntrySize;
    eduProgramQueue.resize(nQueueEntries);

    auto buffer = std::array<std::byte, serial::serialSize<EduQueueEntry>>();

    // TODO: Test all of this
    std::size_t index = 0;
    for(auto & entry : eduProgramQueue)
    {
        auto commandQueueEntrySubstring = command.substr(index * queueEntrySize, queueEntrySize);
        std::transform(std::begin(commandQueueEntrySubstring),
                       std::end(commandQueueEntrySubstring),
                       std::begin(buffer),
                       [](char c) { return Byte(c); });
        entry = serial::Deserialize<EduQueueEntry>(buffer);

        RODOS::PRINTF("Prog ID      : %d\n", static_cast<int>(entry.programId.get()));
        RODOS::PRINTF("Queue ID     : %d\n", static_cast<int>(entry.queueId.get()));
        RODOS::PRINTF("Start Time   : %d\n", static_cast<int>(entry.startTime.get()));
        RODOS::PRINTF("Timeout      : %d\n", static_cast<int>(entry.timeout.get()));

        // Should never be superior or equal to nQueueEntries
        index++;
    }
}
}
