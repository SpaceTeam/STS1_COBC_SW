#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>


namespace sts1cobcsw
{
using sts1cobcsw::serial::Byte;
using sts1cobcsw::serial::Deserialize;
using sts1cobcsw::serial::SerialBuffer;
using sts1cobcsw::serial::serialSize;


auto DispatchCommand(etl::vector<Byte, commandSize> const & command) -> void
{
    auto gsCommandHeader =
        Deserialize<GsCommandHeader>(std::span(command).first<serialSize<GsCommandHeader>>());

    // TODO: Debug print, to be removed
    RODOS::PRINTF("Header start character : %c\n", gsCommandHeader.startCharacter);
    RODOS::PRINTF("Header commandID       : %d\n", gsCommandHeader.commandId);
    RODOS::PRINTF("Header utc             : %d\n", gsCommandHeader.utc);
    RODOS::PRINTF("Header length          : %d\n", gsCommandHeader.length);

    // TODO: Move this somewhere else
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
                BuildEduQueue(std::span(command).subspan<serialSize<GsCommandHeader>>());
                return;
            }
            default:
            {
                RODOS::PRINTF("*Error, invalid command*\n");
                return;
            }
        }
    }

    RODOS::PRINTF("Not implemented yet.\n");
}


auto BuildEduQueue(std::span<Byte const> commandData) -> void
{
    RODOS::PRINTF("Entering build queue command parsing\n");

    eduProgramQueue.clear();
    ParseAndAddQueueEntries(commandData);
    queueIndex = 0U;

    RODOS::PRINTF("Queue index reset. Current size of EDU program queue is %d.\n",
                  static_cast<int>(eduProgramQueue.size()));

    ResumeEduProgramQueueThread();
}


//! @brief Parse and add queue entries from a given span of bytes
//!
//! This function takes a span of bytes containing serialized EDU queue entries, deserializes them,
//! and adds them to the global EDU program queue.
auto ParseAndAddQueueEntries(std::span<Byte const> queueEntries) -> void
{
    RODOS::PRINTF("Printing and parsing\n");

    auto const nQueueEntries = queueEntries.size() / serialSize<EduQueueEntry>;
    eduProgramQueue.resize(nQueueEntries);

    std::size_t index = 0;
    for(auto & entry : eduProgramQueue)
    {
        // TODO: is this the way serial library is intended to be used (lack of SerialBuffer)
        auto offset = index * serialSize<EduQueueEntry>;
        auto entryBuffer = std::span<const Byte, serialSize<EduQueueEntry>>(
            queueEntries.data() + offset, serialSize<EduQueueEntry>);
        // entry = serial::DeserializeConst<EduQueueEntry>(entryBuffer);
        entry = Deserialize<EduQueueEntry>(entryBuffer);

        RODOS::PRINTF("Prog ID      : %d\n", entry.programId.get());
        RODOS::PRINTF("Queue ID     : %d\n", entry.queueId.get());
        RODOS::PRINTF("Start Time   : %d\n", entry.startTime.get());
        RODOS::PRINTF("Timeout      : %d\n", entry.timeout.get());

        // Should never be greater or equal to nQueueEntries
        index++;
    }
}
}
