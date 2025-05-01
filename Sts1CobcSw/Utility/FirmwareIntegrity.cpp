#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Utility/FirmwareIntegrity.hpp>

#include <littlefs/lfs_util.h>

#include <cstdint>


namespace sts1cobcsw::utility
{
constexpr auto maxFirmwareLength = 0x2'0000ULL;
constexpr std::uint32_t initialCrc32JamCrcValue = 0xFFFF'FFFFU;


// Partition structure:
// - Length: without CRC at the end, 4 bytes in little endian
// - Data
// - Checksum: CRC-32 over length + data in little endian
auto CheckFirmwareIntegrity(Partition partition) -> Result<void>
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    auto * partitionUint32Pointer = reinterpret_cast<std::uint32_t volatile *>(partition);
    auto length = static_cast<std::uint32_t>(*partitionUint32Pointer);

    if(length > maxFirmwareLength)
    {
        return ErrorCode::invalidLength;
    }
    if(length == 0)
    {
        return ErrorCode::empty;
    }

    auto crc = lfs_crc(initialCrc32JamCrcValue, &length, sizeof(std::uint32_t));
    auto partitionLength = static_cast<std::uint32_t>(length + 4);  // Add 4 for CRC

    // Read word by word to not discard the volitale qualifier
    auto wordCount = static_cast<std::uint32_t>(partitionLength / sizeof(std::uint32_t));
    for(std::uint32_t offset = 1; offset < wordCount; offset++)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto tempValue = static_cast<std::uint32_t>(*(partitionUint32Pointer + offset));
        crc = lfs_crc(crc, &tempValue, sizeof(tempValue));
    }

    // Read the remaining bytes if partition is not aligned
    auto partitionMisalignment =
        static_cast<std::uint32_t>(partitionLength % sizeof(std::uint32_t));
    if(partitionMisalignment != 0)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
        auto * partitionBytePointer = reinterpret_cast<std::uint8_t volatile *>(partition);
        for(std::uint32_t offset = partitionLength - partitionMisalignment;
            (offset < partitionLength);
            offset++)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto tempValue = *(partitionBytePointer + offset);
            crc = lfs_crc(crc, &tempValue, sizeof(tempValue));
        }
    }

    // Computing checksum over length + data + CRC-32 should be 0
    if(crc != 0)
    {
        return ErrorCode::corrupt;
    }
    return outcome_v2::success();
}
}
