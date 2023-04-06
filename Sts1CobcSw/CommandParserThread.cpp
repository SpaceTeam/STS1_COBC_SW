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

#include <rodos-assert.h>
#include <timemodel.h>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>
#include <etl/vector.h>

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
// constexpr std::size_t commandSize = 30;
// TODO: Use serialSize<EduQueueEntry> instead
// constexpr std::size_t queueEntrySize =
//    sizeof(EduQueueEntry::programId) + sizeof(EduQueueEntry::queueId)
//    + sizeof(EduQueueEntry::startTime) + sizeof(EduQueueEntry::timeout);

// Not sure about this
using Command = etl::vector<Byte, commandSize>;

auto ParseAndAddQueueEntries(etl::vector<Byte, commandSize> const & command) -> void;
auto DispatchComamnd(Command const & command) -> void;


auto ParseAndAddQueueEntries(etl::vector<Byte, commandSize> const & queueEntries) -> void
{
}


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
        constexpr serial::Byte startCharacter{'$'};

        // TODO: The command is not a string. Turn this into an array of bytes, or if different
        // commands have different size, use an etl::vector<Byte>
        auto command = Command();
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
                if(static_cast<Byte>(readCharacter) == startCharacter)
                {
                    startWasDetected = true;
                    command.clear();
                    command.push_back(startCharacter);
                }
                else if(startWasDetected)
                {
                    command.push_back(static_cast<Byte>(readCharacter));
                    // Every command has the same size for now
                    if(command.full())
                    {
                        // TODO: rename this to smthg like "handle command"
                        DispatchCommand(command);
                        startWasDetected = false;
                    }
                }
            }
            RODOS::uart_stdout.suspendUntilDataReady();
        }
    }
} commandParserThread;


/*
auto DispatchCommand(Command const & command) -> void
{
    // Do nothing
    auto buffer = std::array<std::byte, serial::serialSize<GsCommandHeader>>();
    std::copy(std::begin(command), std::end(command), std::begin(buffer));
    auto gsCommandHeader = serial::Deserialize<GsCommandHeader>(buffer);

    RODOS::sysTime.setUTC(utility::UnixToRodosTime(gsCommandHeader.utc));
    utility::PrintFormattedSystemUtc();

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
                return;

                auto const nbQueueEntries =
                    gsCommandHeader.length / static_cast<int>(queueEntrySize);
                RODOS::PRINTF("Number of queue entries : %d\n", nbQueueEntries);

                // Erase all previous entries in the EDU program queue
                eduProgramQueue.clear();

                // FIXME: I think this is the wrong size because serialSize<T> != sizeof(T) in this
                // case.
                auto const queueEntries = std::span(command).subspan(sizeof(GsCommandHeader),
                                                            static_cast<std::size_t>(gsCommandHeader.length));
                ParseAndAddQueueEntries(queueEntries);
                //ParseAndAddQueueEntries(command.substr(
                 //   sizeof(GsCommandHeader), static_cast<std::size_t>(gsCommandHeader.length)));

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


}*/

// constexpr auto headerSize = serial::SerialSize<GsCommandHeader>;

// TODO: Test all of this
/*
auto ParseAndAddQueueEntries(etl::string<commandSize> const & command) -> void
{
    auto const nQueueEntries = command.size() / queueEntrySize;
    eduProgramQueue.resize(nQueueEntries);

    auto buffer = SerialBuffer<EduQueueEntry>{};

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
}*/
}
