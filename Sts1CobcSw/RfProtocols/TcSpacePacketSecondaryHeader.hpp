#pragma once


#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <bit>
#include <cstddef>


namespace sts1cobcsw
{
namespace tc
{
struct SpacePacketSecondaryHeader
{
    UInt<4> tcPacketPusVersionNumber;
    UInt<4> acknowledgementFlags;
    MessageTypeId messageTypeId;
    ApplicationProcessUserId sourceId;
};


// I didn't want to create an .ipp file just for this function
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, SpacePacketSecondaryHeader * header)
    -> void const *
{
    source = DeserializeFrom<endianness>(
        source, &header->tcPacketPusVersionNumber, &header->acknowledgementFlags);
    source = DeserializeFrom<endianness>(source, &header->messageTypeId);
    source = DeserializeFrom<endianness>(source, &header->sourceId);
    return source;
}
}


template<>
inline constexpr std::size_t serialSize<tc::SpacePacketSecondaryHeader> =
    totalSerialSize<decltype(tc::SpacePacketSecondaryHeader::tcPacketPusVersionNumber),
                    decltype(tc::SpacePacketSecondaryHeader::acknowledgementFlags)>
    + totalSerialSize<decltype(tc::SpacePacketSecondaryHeader::messageTypeId),
                      decltype(tc::SpacePacketSecondaryHeader::sourceId)>;
static_assert(serialSize<tc::SpacePacketSecondaryHeader> == tc::packetSecondaryHeaderLength);
}
