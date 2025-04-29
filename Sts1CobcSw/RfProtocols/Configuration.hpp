#pragma once


#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <array>
#include <cstdint>


// TODO: Choose the values
namespace sts1cobcsw
{
inline constexpr auto ccsdsEndianness = std::endian::big;


// --- Synchronization and channel coding ---

namespace ecc
{
// RS(255, 223)
inline constexpr auto blockLength = 255;
inline constexpr auto messageLength = 223;
inline constexpr auto nParitySymbols = 32;
static_assert(nParitySymbols == blockLength - messageLength);

using Block = std::array<Byte, blockLength>;
}


// --- Space Data Link Protocol ---

namespace tm
{
inline constexpr auto transferFrameVersionNumber = UInt<2>(0);
inline constexpr auto transferFrameLength = ecc::messageLength;
inline constexpr auto transferFramePrimaryHeaderLength = 6;
inline constexpr auto transferFrameDataLength =
    transferFrameLength - transferFramePrimaryHeaderLength;
}

namespace tc
{
inline constexpr auto transferFrameVersionNumber = UInt<2>(0);
inline constexpr auto transferFrameLength = ecc::messageLength;
inline constexpr auto transferFramePrimaryHeaderLength = 5;
inline constexpr auto transferFrameDataLength =
    transferFrameLength - transferFramePrimaryHeaderLength;
}

using SpacecraftId = Id<UInt<10>, 0x123>;  // NOLINT(*magic-numbers)
inline constexpr auto spacecraftId = SpacecraftId::Make<0x123>();

using Vcid = Id<UInt<3>, 0b011, 0b101>;  // NOLINT(*magic-numbers)
inline constexpr auto pusVcid = Vcid::Make<0b011>();
inline constexpr auto cfdpVcid = Vcid::Make<0b101>();


// --- Space Packet Protocol ---

struct PacketType  // I use a struct instead of a namespace because I don't like "packettype"
{
    static constexpr auto telemetry = UInt<1>(0);
    static constexpr auto telecommand = UInt<1>(1);
};


inline constexpr auto packetVersionNumber = UInt<3>(0);
inline constexpr auto maxPacketLength = tm::transferFrameDataLength;
inline constexpr auto packetPrimaryHeaderLength = 6U;
inline constexpr auto maxPacketDataLength = maxPacketLength - packetPrimaryHeaderLength;

using Apid = Id<UInt<11>, 0, 0b000'1100'1100, 0x7FF>;  // NOLINT(*magic-numbers)
inline constexpr auto invalidApid = Apid::Make<0>();
inline constexpr auto normalApid = Apid::Make<0b000'1100'1100>();
inline constexpr auto idlePacketApid = Apid::Make<0x7FF>();

inline constexpr auto idleData = 0x55_b;
}
