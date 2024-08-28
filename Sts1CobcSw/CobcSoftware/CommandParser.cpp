#include <Sts1CobcSw/CobcSoftware/CommandParser.hpp>
#include <Sts1CobcSw/CobcSoftware/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/ProgramQueue.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Debug.hpp>

#include <strong_type/type.hpp>

#include <cinttypes>  // IWYU pragma: keep


namespace sts1cobcsw
{


auto DispatchCommand(etl::vector<Byte, commandSize> const & command) -> void
{
    auto gsCommandHeader =
        Deserialize<GsCommandHeader>(std::span(command).first<totalSerialSize<GsCommandHeader>>());

    DEBUG_PRINT("Header start character : %c\n", gsCommandHeader.startCharacter);
    DEBUG_PRINT("Header commandID       : %" PRIi8 "\n", gsCommandHeader.commandId);
    DEBUG_PRINT("Header utc             : %" PRIi32 "\n", gsCommandHeader.utc);
    DEBUG_PRINT("Header length          : %" PRIi16 "\n", gsCommandHeader.length);

    // TODO: Move this somewhere else
    // TODO: Compute real time offset from command UTC
    DEBUG_PRINT_REAL_TIME();

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
                BuildEduQueue(std::span(command).subspan<totalSerialSize<GsCommandHeader>>());
                return;
            }
            default:
            {
                DEBUG_PRINT("*Error, invalid command*\n");
                return;
            }
        }
    }

    DEBUG_PRINT("Not implemented yet.\n");
}


auto BuildEduQueue(std::span<Byte const> commandData) -> void
{
    DEBUG_PRINT("Entering build queue command parsing\n");

    edu::programQueue.clear();
    ParseAndAddQueueEntries(commandData);
    edu::queueIndex = 0;

    DEBUG_PRINT("Queue index reset. Current size of EDU program queue is %d.\n",
                static_cast<int>(edu::programQueue.size()));

    ResumeEduProgramQueueThread();
}


//! @brief Parse and add queue entries from a given span of bytes
//!
//! This function takes a span of bytes containing serialized EDU queue entries, deserializes them,
//! and adds them to the global EDU program queue.
auto ParseAndAddQueueEntries(std::span<Byte const> queueEntries) -> void
{
    DEBUG_PRINT("Printing and parsing\n");

    while(queueEntries.size() >= totalSerialSize<edu::ProgramQueueEntry>
          and (not edu::programQueue.full()))
    {
        auto entry = Deserialize<edu::ProgramQueueEntry>(
            queueEntries.first<totalSerialSize<edu::ProgramQueueEntry>>());

        DEBUG_PRINT("Prog ID      : %" PRIu16 "\n", value_of(entry.programId));
        DEBUG_PRINT("Start Time   : %" PRIi32 "\n", value_of(entry.startTime));
        DEBUG_PRINT("Timeout      : %" PRIi16 "\n", entry.timeout);

        edu::programQueue.push_back(entry);
        queueEntries = queueEntries.subspan<totalSerialSize<edu::ProgramQueueEntry>>();
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
