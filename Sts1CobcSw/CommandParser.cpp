#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <type_safe/types.hpp>

using sts1cobcsw::serial::Byte;
using sts1cobcsw::serial::Deserialize;
using sts1cobcsw::serial::SerialBuffer;
using sts1cobcsw::serial::serialSize;


namespace sts1cobcsw
{
auto DispatchCommand(etl::vector<Byte, commandSize> const & command) -> void
{
    auto commandHeader = std::span<const Byte, serial::serialSize<GsCommandHeader>>(
        command.data(), serialSize<GsCommandHeader>);
    auto commandData =
        std::span<const Byte>{command.data() + serialSize<GsCommandHeader>, dataSize};

    auto gsCommandHeader = serial::DeserializeConst<GsCommandHeader>(commandHeader);

    // Debug print
    RODOS::PRINTF("Header start character : %c\n", gsCommandHeader.startCharacter);
    RODOS::PRINTF("Header comamndID       : %d\n", gsCommandHeader.commandId);
    RODOS::PRINTF("Header utc             : %d\n", gsCommandHeader.utc);
    RODOS::PRINTF("Header length          : %d\n", gsCommandHeader.length);

    // HandleUTC
    RODOS::sysTime.setUTC(utility::UnixToRodosTime(gsCommandHeader.utc));
    utility::PrintFormattedSystemUtc();

    RODOS::PRINTF("Done for retrieving header\n");
    RODOS::PRINTF("Handling command\n");

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
                BuildEduQueue(commandData);
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

auto BuildEduQueue(std::span<const Byte> commandData) -> void
{
    RODOS::PRINTF("Entering build queue command parsing\n");

    eduProgramQueue.clear();

    ParseAndAddQueueEntries(commandData);

    // Reset queue index
    queueIndex = 0U;
    RODOS::PRINTF("Queue index reset. Current size of EDU program queue is %d.\n",
                  static_cast<int>(eduProgramQueue.size()));

    ResumeEduQueueThread();
}


auto ParseAndAddQueueEntries(std::span<const Byte> & queueEntries) -> void
{
    RODOS::PRINTF("Printing and parsing\n");

    auto const nQueueEntries = queueEntries.size() / queueEntrySize;
    eduProgramQueue.resize(nQueueEntries);

    auto buffer = SerialBuffer<EduQueueEntry>();

    std::size_t index = 0;
    for(auto & entry : eduProgramQueue)
    {
        auto queueEntrySpan = queueEntries.subspan(index * queueEntrySize, queueEntrySize);
        std::copy_n(queueEntrySpan.begin(), queueEntrySize, std::begin(buffer));
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
