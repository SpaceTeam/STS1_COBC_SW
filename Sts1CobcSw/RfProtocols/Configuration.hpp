#pragma once


#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/MessageTypeIdFields.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>

#include <algorithm>
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
inline constexpr auto securityHeaderLength = 4;   // TODO: What's the right number?
inline constexpr auto securityTrailerLength = 8;  // TODO: What's the right number?
inline constexpr auto transferFrameDataLength = transferFrameLength
                                              - transferFramePrimaryHeaderLength
                                              - securityHeaderLength - securityTrailerLength;
}

using SpacecraftId = Id<UInt<10>, 0x123>;  // NOLINT(*magic-numbers)
inline constexpr auto spacecraftId = Make<SpacecraftId, 0x123>();

using Vcid = Id<UInt<3>, 0b011, 0b101>;  // NOLINT(*magic-numbers)
inline constexpr auto pusVcid = Make<Vcid, 0b011>();
inline constexpr auto cfdpVcid = Make<Vcid, 0b101>();


// --- Space Packet Protocol ---

inline constexpr auto packetVersionNumber = UInt<3>(0);
inline constexpr auto packetPrimaryHeaderLength = 6U;

namespace tm
{
inline constexpr auto maxPacketLength = transferFrameDataLength;
inline constexpr auto maxPacketDataLength = maxPacketLength - packetPrimaryHeaderLength;
}

namespace tc
{
inline constexpr auto maxPacketLength = transferFrameDataLength;
inline constexpr auto maxPacketDataLength = maxPacketLength - packetPrimaryHeaderLength;
}

using Apid = Id<UInt<11>, 0b000'1100'1100, 0x7FF>;  // NOLINT(*magic-numbers)
inline constexpr auto normalApid = Make<Apid, 0b000'1100'1100>();
inline constexpr auto idlePacketApid = Make<Apid, 0x7FF>();

inline constexpr auto idleData = 0x55_b;


// --- Packet Utilization Standard ---

namespace tm
{
inline constexpr auto packetSecondaryHeaderLength = 11U;
inline constexpr auto packetPusVersionNumber = UInt<4>(2);

// NOLINTBEGIN(*magic-numbers)
using MessageTypeId = Id<MessageTypeIdFields,
                         MessageTypeIdFields{1, 1},
                         MessageTypeIdFields{1, 2},
                         MessageTypeIdFields{1, 7},
                         MessageTypeIdFields{1, 8},
                         MessageTypeIdFields{3, 25},
                         MessageTypeIdFields{6, 6},
                         MessageTypeIdFields{20, 2},
                         MessageTypeIdFields{23, 4},
                         MessageTypeIdFields{23, 13}>;
// NOLINTEND(*magic-numbers)
}

namespace tc
{
inline constexpr auto packetPusVersionNumber = UInt<4>(2);
}

using ApplicationProcessUserId = Id<std::uint16_t, 0xAA33>;  // NOLINT(*magic-numbers)
inline constexpr auto applicationProcessUserId = Make<ApplicationProcessUserId, 0xAA33>();

enum class ParameterId : std::uint8_t
{
    rxBaudRate = 1,
    txBaudRate,
    realTimeOffsetCorrection,
    newEduResultIsAvailable,
    eduStartDelayLimit,
};
using ParameterValue = std::uint32_t;
inline constexpr auto maxNParameters = 5U;  // Chosen to be the number of different parameters

enum class FileStatus : std::uint8_t
{
    unlocked = 0,
    locked = 0xFF,
};

enum class ObjectType : std::uint8_t
{
    file = 0,
    directory = 1,
};

inline constexpr auto maxDumpedDataLength =
    tm::maxPacketDataLength - tm::packetSecondaryHeaderLength
    - totalSerialSize<std::uint8_t, fram::Address, std::uint8_t>;
}
