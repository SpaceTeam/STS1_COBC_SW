#pragma once


#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Payload.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <bit>


namespace sts1cobcsw
{
struct SpacePacketPrimaryHeader
{
    UInt<3> versionNumber = sts1cobcsw::packetVersionNumber;
    UInt<1> packetType = 0;
    UInt<1> secondaryHeaderFlag = 0;
    Apid apid;
    UInt<2> sequenceFlags = 0b11;
    UInt<14> packetSequenceCount = 0;  // NOLINT(*magic-numbers)
    std::uint16_t packetDataLength = 0;
};


struct SpacePacket
{
    using PrimaryHeader = SpacePacketPrimaryHeader;

    PrimaryHeader primaryHeader;
    std::span<Byte const> dataField;
};


template<>
inline constexpr auto serialSize<SpacePacketPrimaryHeader> =
    totalSerialSize<decltype(SpacePacketPrimaryHeader::versionNumber),
                    decltype(SpacePacketPrimaryHeader::packetType),
                    decltype(SpacePacketPrimaryHeader::secondaryHeaderFlag),
                    decltype(SpacePacketPrimaryHeader::apid.Value()),
                    decltype(SpacePacketPrimaryHeader::sequenceFlags),
                    decltype(SpacePacketPrimaryHeader::packetSequenceCount)>
    + totalSerialSize<decltype(SpacePacketPrimaryHeader::packetDataLength)>;
static_assert(serialSize<SpacePacketPrimaryHeader> == packetPrimaryHeaderLength);


[[nodiscard]] auto AddSpacePacketTo(etl::ivector<Byte> * dataField,
                                    bool hasSecondaryHeader,
                                    Apid apid,
                                    Payload const & payload) -> Result<void>;
[[nodiscard]] auto ParseAsSpacePacket(std::span<Byte const> buffer) -> Result<SpacePacket>;


template<std::endian endianness>
auto SerializeTo(void * destination, SpacePacketPrimaryHeader const & header) -> void *;
template<std::endian endianness>
auto DeserializeFrom(void const * source, SpacePacketPrimaryHeader * header) -> void const *;
}
