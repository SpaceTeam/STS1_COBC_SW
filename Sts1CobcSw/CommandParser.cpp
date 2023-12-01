#include <Sts1CobcSw/CommandParser.hpp>
#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>

#include <rodos_no_using_namespace.h>

#include <cinttypes>


namespace sts1cobcsw
{


auto DispatchCommand(etl::vector<Byte, commandSize> const & command) -> void
{
    auto gsCommandHeader =
        Deserialize<GsCommandHeader>(std::span(command).first<serialSize<GsCommandHeader>>());

    // TODO: Debug print, to be removed
    RODOS::PRINTF("Header start character : %c\n", gsCommandHeader.startCharacter);
    RODOS::PRINTF("Header commandID       : %" PRIi8 "\n", gsCommandHeader.commandId);
    RODOS::PRINTF("Header utc             : %" PRIi32 "\n", gsCommandHeader.utc);
    RODOS::PRINTF("Header length          : %" PRIi16 "\n", gsCommandHeader.length);

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
                edu::TurnOn();
                return;
            }
            case CommandId::turnEduOff:
            {
                edu::TurnOff();
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

    edu::programQueue.clear();
    ParseAndAddQueueEntries(commandData);
    edu::queueIndex = 0U;

    RODOS::PRINTF("Queue index reset. Current size of EDU program queue is %d.\n",
                  static_cast<int>(edu::programQueue.size()));

    ResumeEduProgramQueueThread();
}


//! @brief Parse and add queue entries from a given span of bytes
//!
//! This function takes a span of bytes containing serialized EDU queue entries, deserializes them,
//! and adds them to the global EDU program queue.
auto ParseAndAddQueueEntries(std::span<Byte const> queueEntries) -> void
{
    RODOS::PRINTF("Printing and parsing\n");

    while(queueEntries.size() >= serialSize<edu::QueueEntry> and (not edu::programQueue.full()))
    {
        auto entry =
            Deserialize<edu::QueueEntry>(queueEntries.first<serialSize<edu::QueueEntry>>());

        RODOS::PRINTF("Prog ID      : %" PRIu16 "\n", entry.programId);
        RODOS::PRINTF("Timestamp    : %" PRIu32 "\n", entry.timestamp);
        RODOS::PRINTF("Start Time   : %" PRIu32 "\n", entry.startTime);
        RODOS::PRINTF("Timeout      : %" PRIi16 "\n", entry.timeout);

        edu::programQueue.push_back(entry);
        queueEntries = queueEntries.subspan<serialSize<edu::QueueEntry>>();
    }
}


template<std::endian endianness>
auto DeserializeFrom(void const * source, GsCommandHeader * data) -> void const *
{
    source = DeserializeFrom<endianness>(source, &(data->startCharacter));
    source = DeserializeFrom<endianness>(source, &(data->utc));
    source = DeserializeFrom<endianness>(source, &(data->commandId));
    source = DeserializeFrom<endianness>(source, &(data->length));
    return source;
}

// Explicit template specializations to keep everything in .cpp file
template auto DeserializeFrom<std::endian::big>(void const *, GsCommandHeader *) -> void const *;
template auto DeserializeFrom<std::endian::little>(void const *, GsCommandHeader *) -> void const *;


template<std::endian endianness>
auto SerializeTo(void * destination, GsCommandHeader const & data) -> void *
{
    destination = SerializeTo<endianness>(destination, data.startCharacter);
    destination = SerializeTo<endianness>(destination, data.utc);
    destination = SerializeTo<endianness>(destination, data.commandId);
    destination = SerializeTo<endianness>(destination, data.length);
    return destination;
}

// Explicit template specializations to keep everything in .cpp file
template auto SerializeTo<std::endian::big>(void *, GsCommandHeader const &) -> void *;
template auto SerializeTo<std::endian::little>(void *, GsCommandHeader const &) -> void *;
}
