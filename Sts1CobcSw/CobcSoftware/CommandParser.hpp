#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <etl/vector.h>

#include <bit>
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
    char startCharacter = '\0';
    std::int32_t utc = 0;
    std::int8_t commandId = 0;
    std::int16_t length = 0;
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
    totalSerialSize<GsCommandHeader> + 2 * totalSerialSize<edu::ProgramQueueEntry>;


auto DispatchCommand(etl::vector<Byte, commandSize> const & command) -> void;
auto ParseAndAddQueueEntries(std::span<Byte const> queueEntries) -> void;
auto BuildEduQueue(std::span<Byte const> commandData) -> void;

template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, GsCommandHeader * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, GsCommandHeader const & data) -> void *;
}
