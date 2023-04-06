#pragma once

#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>


namespace sts1cobcsw
{
using serial::Byte;
using sts1cobcsw::serial::SerialBuffer;

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

// TODO: Remove hardcoded values
// [startCharacter + UTC + commandId + length] + [data]
constexpr std::size_t commandSize = 30;
constexpr std::size_t dataSize = 22;
constexpr std::size_t queueEntrySize =
    sizeof(EduQueueEntry::programId) + sizeof(EduQueueEntry::queueId)
    + sizeof(EduQueueEntry::startTime) + sizeof(EduQueueEntry::timeout);


auto DispatchCommand(etl::vector<Byte, commandSize> const & command) -> void;
auto ParseAndAddQueueEntries(std::span<Byte, dataSize> & queueEntries) -> void;
auto ParseAndAddQueueEntries(std::span<const Byte> & queueEntries) -> void;
void BuildEduQueue(std::span<const Byte> commandData);

namespace serial
{
template<>
inline constexpr std::size_t serialSize<sts1cobcsw::GsCommandHeader> =
    totalSerialSize<decltype(GsCommandHeader::startCharacter),
                    decltype(GsCommandHeader::utc),
                    decltype(GsCommandHeader::commandId),
                    decltype(GsCommandHeader::length)>;
}

inline auto DeserializeFromConst(const Byte * source, GsCommandHeader * data) -> const Byte *
{
    source = serial::DeserializeFromConst(source, &(data->startCharacter));
    source = serial::DeserializeFromConst(source, &(data->utc));
    source = serial::DeserializeFromConst(source, &(data->commandId));
    source = serial::DeserializeFromConst(source, &(data->length));
    return source;
}

inline auto DeserializeFromConst(const Byte * source, EduQueueEntry * data) -> const Byte *
{
    source = serial::DeserializeFromConst(source, &(data->programId));
    source = serial::DeserializeFromConst(source, &(data->queueId));
    source = serial::DeserializeFromConst(source, &(data->startTime));
    source = serial::DeserializeFromConst(source, &(data->timeout));
    return source;
}
}
