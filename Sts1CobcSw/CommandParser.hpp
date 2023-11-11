#include <Sts1CobcSw/Edu/ProgramQueue.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <etl/vector.h>

#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw
{

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


template<>
inline constexpr std::size_t serialSize<GsCommandHeader> =
    totalSerialSize<decltype(GsCommandHeader::startCharacter),
                    decltype(GsCommandHeader::utc),
                    decltype(GsCommandHeader::commandId),
                    decltype(GsCommandHeader::length)>;

// TODO: Choose a proper value for the commandSize. Right now this is just size required by the
// DispatchCommand() test.
inline constexpr std::size_t commandSize =
    serialSize<GsCommandHeader> + 2 * serialSize<edu::QueueEntry>;


auto DispatchCommand(etl::vector<Byte, commandSize> const & command) -> void;
auto ParseAndAddQueueEntries(std::span<Byte const> queueEntries) -> void;
auto BuildEduQueue(std::span<Byte const> commandData) -> void;
auto DeserializeFrom(void const * source, GsCommandHeader * data) -> void const *;
auto SerializeTo(void * destination, GsCommandHeader const & data) -> void *;
}
