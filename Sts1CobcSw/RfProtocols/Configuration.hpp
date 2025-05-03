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
inline constexpr auto spacecraftId = Make<SpacecraftId, 0x123>();

using Vcid = Id<UInt<3>, 0b011, 0b101>;  // NOLINT(*magic-numbers)
inline constexpr auto pusVcid = Make<Vcid, 0b011>();
inline constexpr auto cfdpVcid = Make<Vcid, 0b101>();


// --- Space Packet Protocol ---

inline constexpr auto packetVersionNumber = UInt<3>(0);
inline constexpr auto maxPacketLength = tm::transferFrameDataLength;
inline constexpr auto packetPrimaryHeaderLength = 6U;
inline constexpr auto maxPacketDataLength = maxPacketLength - packetPrimaryHeaderLength;

using Apid = Id<UInt<11>, 0b000'1100'1100, 0x7FF>;  // NOLINT(*magic-numbers)
inline constexpr auto normalApid = Make<Apid, 0b000'1100'1100>();
inline constexpr auto idlePacketApid = Make<Apid, 0x7FF>();

inline constexpr auto idleData = 0x55_b;
}
