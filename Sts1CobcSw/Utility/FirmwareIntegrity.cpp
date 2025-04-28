#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/FirmwareIntegrity.hpp>


namespace sts1cobcsw::utility
{
constexpr auto maxFirmwareLength = 0x2'0000ULL;


// Partition structure:
// - Length: without CRC at the end, 4 bytes in little endian
// - Data
// - Checksum: CRC-32 over length + data in little endian
auto CheckFirmwareIntegrity(Partition partition) -> Result<void>
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    auto * startAddress = reinterpret_cast<void *>(partition);
    auto length = 0U;
    std::memcpy(&length, startAddress, sizeof(length));
    if(length > maxFirmwareLength)
    {
        return ErrorCode::invalidLength;
    }
    if(length == 0)
    {
        return ErrorCode::empty;
    }
    // Computing checksum over length + data + CRC-32 should be 0
    auto firmwareSpan = std::span<Byte const>{static_cast<Byte const *>(startAddress), length + 4};
    auto crc = utility::ComputeCrc32(firmwareSpan);
    if(crc != 0)
    {
        return ErrorCode::corrupt;
    }
    return outcome_v2::success();
}
}
