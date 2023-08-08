#include <Sts1CobcSw/Edu/ProgramQueue.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <etl/vector.h>

#include <cstdint>
#include <span>


namespace sts1cobcsw
{
using serial::Byte;
using sts1cobcsw::serial::DeserializeFrom;
using sts1cobcsw::serial::SerializeTo;


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


namespace serial
{
template<>
inline constexpr std::size_t serialSize<GsCommandHeader> =
    totalSerialSize<decltype(GsCommandHeader::startCharacter),
                    decltype(GsCommandHeader::utc),
                    decltype(GsCommandHeader::commandId),
                    decltype(GsCommandHeader::length)>;
}

// TODO: Choose a proper value for the commandSize. Right now this is just size required by the
// DispatchCommand() test.
inline constexpr std::size_t commandSize =
    serial::serialSize<GsCommandHeader> + 2 * serial::serialSize<edu::QueueEntry>;


auto DispatchCommand(etl::vector<Byte, commandSize> const & command) -> void;
auto ParseAndAddQueueEntries(std::span<Byte const> queueEntries) -> void;
auto BuildEduQueue(std::span<Byte const> commandData) -> void;


// TODO: Turn into noninline function
inline auto DeserializeFrom(void const * source, GsCommandHeader * data) -> void const *
{
    source = DeserializeFrom(source, &(data->startCharacter));
    source = DeserializeFrom(source, &(data->utc));
    source = DeserializeFrom(source, &(data->commandId));
    source = DeserializeFrom(source, &(data->length));
    return source;
}


// TODO: Turn into noninline function
inline auto SerializeTo(void * destination, GsCommandHeader const & data) -> void *
{
    destination = SerializeTo(destination, data.startCharacter);
    destination = SerializeTo(destination, data.utc);
    destination = SerializeTo(destination, data.commandId);
    destination = SerializeTo(destination, data.length);
    return destination;
}
}
