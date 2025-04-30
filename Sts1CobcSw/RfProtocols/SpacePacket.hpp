#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/Payload.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <etl/vector.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw
{
namespace packettype
{
constexpr auto telemetry = UInt<1>(0);
constexpr auto telecommand = UInt<1>(1);
};


// TODO: Consider moving this into SpacePacket
struct SpacePacketPrimaryHeader
{
    UInt<3> versionNumber = sts1cobcsw::packetVersionNumber;
    UInt<1> packetType;
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
[[nodiscard]] auto SerializeTo(void * destination, SpacePacketPrimaryHeader const & header)
    -> void *;
template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, SpacePacketPrimaryHeader * header)
    -> void const *;
}
