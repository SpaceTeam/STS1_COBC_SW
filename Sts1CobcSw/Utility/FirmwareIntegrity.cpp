#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/FirmwareIntegrity.hpp>


namespace sts1cobcsw::utility
{
constexpr auto maxFirmwareLength = 0x20000ULL;

auto CheckFirmwareIntegrity(Partition partition) -> Result<void>
{
    // Partition structure:
    // First 4 Bytes length without crc at the end in little endian
    // Partition data
    // 4 Byte crc over length and partition data with initial Value 0x00 in little endian

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    auto * startPointer = reinterpret_cast<void *>(partition);
    auto length = 0U;
    std::memcpy(&length, startPointer, sizeof(length));
    if(length > maxFirmwareLength)
    {
        return ErrorCode::invalidLength;
    }
    if(length == 0)
    {
        return ErrorCode::empty;
    }
    // Add last 4 bytes for existing crc32 at the end of the partition
    auto firmwareSpan = std::span<Byte const>{static_cast<Byte const *>(startPointer), length + 4};

    // Using same crc logic and initial value as the bootloader uses
    if(utility::ComputeCrc32(0x00U, firmwareSpan) != 0x00U)
    {
        return ErrorCode::corrupt;
    }
    return outcome_v2::success();
}
}
