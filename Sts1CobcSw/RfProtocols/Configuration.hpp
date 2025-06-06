#pragma once


#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/Vocabulary.hpp>
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
inline constexpr auto securityHeaderLength = 2;
inline constexpr auto securityTrailerLength = 8;
inline constexpr auto transferFrameDataLength = transferFrameLength
                                              - transferFramePrimaryHeaderLength
                                              - securityHeaderLength - securityTrailerLength;
}

inline constexpr std::uint16_t securityParameterIndex = 0x1717;

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
inline constexpr auto maxMessageDataLength = maxPacketDataLength - packetSecondaryHeaderLength;

// NOLINTBEGIN(*magic-numbers)
using MessageTypeId = Id<MessageTypeIdFields,
                         {1, 1},
                         {1, 2},
                         {1, 7},
                         {1, 8},
                         {3, 25},
                         {6, 6},
                         {20, 2},
                         {23, 4},
                         {23, 13}>;
// NOLINTEND(*magic-numbers)
}

namespace tc
{
inline constexpr auto packetSecondaryHeaderLength = 5U;
inline constexpr auto packetPusVersionNumber = UInt<4>(2);
inline constexpr auto maxMessageDataLength = maxPacketDataLength - packetSecondaryHeaderLength;

// NOLINTBEGIN(*magic-numbers)
using MessageTypeId = Id<MessageTypeIdFields,
                         {6, 2},
                         {6, 5},
                         {8, 1},
                         {20, 1},
                         {20, 3},
                         {23, 2},
                         {23, 3},
                         {23, 12},
                         {23, 14}>;
// NOLINTEND(*magic-numbers)

enum class FunctionId : std::uint8_t
{
    stopAntennaDeployment = 1,
    requestHousekeepingParameterReports = 2,
    disableCubeSatTx = 4,
    enableCubeSatTx = 7,
    resetNow = 8,
    enableFileTransferFor = 9,
    updateEduQueue = 22,
    setActiveFirmware = 23,
    setBackupFirmware = 25,
    checkFirmwareIntegrity = 31,
};
enum class FirmwarePartitionId : std::uint8_t
{
    primary = 0b0000'1111,
    secondary1 = 0b0000'0000,
    secondary2 = 0b1111'1111
};
}

using ApplicationProcessUserId = Id<std::uint16_t, 0xAA33>;  // NOLINT(*magic-numbers)
inline constexpr auto applicationProcessUserId = Make<ApplicationProcessUserId, 0xAA33>();

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

using CopyOperationId = Id<std::uint8_t, 0b0000'1111>;  // NOLINT(*magic-numbers)
inline constexpr auto copyOperationId = Make<CopyOperationId, 0b0000'1111>();

inline constexpr auto maxDumpedDataLength =
    tm::maxPacketDataLength - tm::packetSecondaryHeaderLength
    - totalSerialSize<std::uint8_t, fram::Address, std::uint8_t>;
}
