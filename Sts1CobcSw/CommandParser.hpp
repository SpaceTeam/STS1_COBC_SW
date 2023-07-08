#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <type_safe/types.hpp>

#include <etl/vector.h>


namespace sts1cobcsw
{
using serial::Byte;
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


namespace serial
{
template<>
inline constexpr std::size_t serialSize<GsCommandHeader> =
    totalSerialSize<decltype(GsCommandHeader::startCharacter),
                    decltype(GsCommandHeader::utc),
                    decltype(GsCommandHeader::commandId),
                    decltype(GsCommandHeader::length)>;
}


inline auto DeserializeFrom(void const * source, GsCommandHeader * data) -> void const *
{
    source = DeserializeFrom(source, &(data->startCharacter));
    source = DeserializeFrom(source, &(data->utc));
    source = DeserializeFrom(source, &(data->commandId));
    source = DeserializeFrom(source, &(data->length));
    return source;
}


inline constexpr std::size_t commandSize = 30;
inline constexpr std::size_t dataSize = commandSize - serial::serialSize<GsCommandHeader>;


auto DispatchCommand(etl::vector<Byte, commandSize> const & command) -> void;
auto ParseAndAddQueueEntrie(std::span<const Byte> & queueEntries) -> void;
auto BuildEduQueue(std::span<const Byte> commandData) -> void;
}
