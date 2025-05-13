#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RealTime/RealTime.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Payload.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <etl/vector.h>

#include <bit>
#include <cstdint>


namespace sts1cobcsw
{
namespace tm
{
// TODO: Maybe remove the NTTP and make messageTypeId a normal member variable
template<MessageTypeId id>
struct SpacePacketSecondaryHeader
{
    static constexpr auto tmPacketPusVersionNumber = UInt<4>(2);
    static constexpr auto spacecraftTimeReferenceStatus = UInt<4>(0);
    static constexpr auto messageTypeId = id;
    std::uint16_t messageTypeCounter = 0;
    static constexpr auto destinationId = applicationProcessUserId;
    RealTime time = RealTime(0);
};


// I didn't want to create an .ipp file just for this function
template<std::endian endianness, MessageTypeId id>
[[nodiscard]] auto SerializeTo(void * destination, SpacePacketSecondaryHeader<id> const & header)
    -> void *
{
    using sts1cobcsw::SerializeTo;

    destination = SerializeTo<endianness>(
        destination, header.tmPacketPusVersionNumber, header.spacecraftTimeReferenceStatus);
    destination = SerializeTo<endianness>(destination, header.messageTypeId);
    destination = SerializeTo<endianness>(destination, header.messageTypeCounter);
    destination = SerializeTo<endianness>(destination, header.destinationId);
    destination = SerializeTo<endianness>(destination, header.time);
    return destination;
}
}


template<tm::MessageTypeId id>
inline constexpr std::size_t serialSize<tm::SpacePacketSecondaryHeader<id>> =
    totalSerialSize<decltype(tm::SpacePacketSecondaryHeader<id>::tmPacketPusVersionNumber),
                    decltype(tm::SpacePacketSecondaryHeader<id>::spacecraftTimeReferenceStatus)>
    + totalSerialSize<decltype(tm::SpacePacketSecondaryHeader<id>::messageTypeId),
                      decltype(tm::SpacePacketSecondaryHeader<id>::messageTypeCounter),
                      decltype(tm::SpacePacketSecondaryHeader<id>::destinationId),
                      decltype(tm::SpacePacketSecondaryHeader<id>::time)>;
static_assert(serialSize<tm::SpacePacketSecondaryHeader<tm::MessageTypeId{}>>
              == tm::packetSecondaryHeaderLength);
}
