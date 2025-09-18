#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/Payload.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <strong_type/regular.hpp>
#include <strong_type/type.hpp>

#include <etl/vector.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>


namespace sts1cobcsw
{
using PacketType = strong::type<UInt<1>, struct PacketTypeTag, strong::regular>;
constexpr auto telemetryPacketType = PacketType(0);
constexpr auto telecommandPacketType = PacketType(1);


// TODO: Consider moving this into SpacePacket
struct SpacePacketPrimaryHeader
{
    UInt<3> versionNumber = sts1cobcsw::packetVersionNumber;
    PacketType packetType;
    UInt<1> secondaryHeaderFlag;
    Apid apid;
    UInt<2> sequenceFlags;
    UInt<14> packetSequenceCount;  // NOLINT(*magic-numbers)
    std::uint16_t packetDataLength = 0;
};


struct SpacePacket
{
    using PrimaryHeader = SpacePacketPrimaryHeader;

    PrimaryHeader primaryHeader;
    std::span<Byte const> dataField;
};


template<>
inline constexpr std::size_t serialSize<SpacePacketPrimaryHeader> =
    totalSerialSize<decltype(SpacePacketPrimaryHeader::versionNumber),
                    strong::underlying_type_t<PacketType>,
                    decltype(SpacePacketPrimaryHeader::secondaryHeaderFlag),
                    decltype(SpacePacketPrimaryHeader::apid.Value()),
                    decltype(SpacePacketPrimaryHeader::sequenceFlags),
                    decltype(SpacePacketPrimaryHeader::packetSequenceCount)>
    + totalSerialSize<decltype(SpacePacketPrimaryHeader::packetDataLength)>;
static_assert(serialSize<SpacePacketPrimaryHeader> == packetPrimaryHeaderLength);


[[nodiscard]] auto AddSpacePacketTo(etl::ivector<Byte> * dataField,
                                    Apid apid,
                                    Payload const & payload) -> Result<void>;
[[nodiscard]] auto ParseAsSpacePacket(std::span<Byte const> buffer) -> Result<SpacePacket>;


template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, SpacePacketPrimaryHeader const & header)
    -> void *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, SpacePacketPrimaryHeader * header)
    -> void const *;
}
