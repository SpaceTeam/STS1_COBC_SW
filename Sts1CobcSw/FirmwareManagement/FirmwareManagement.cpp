#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>

#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <etl/vector.h>

#include <algorithm>
#ifdef __linux__
    #include <array>
#endif
#include <cstddef>
#include <span>


namespace sts1cobcsw::fw
{
constexpr auto maxFirmwareLength = 0x2'0000U;


#ifdef __linux__
auto primaryPartitionMemory = std::array<std::uint8_t, maxFirmwareLength>{};
auto secondaryPartition1Memory = std::array<std::uint8_t, maxFirmwareLength>{};
auto secondaryPartition2Memory = std::array<std::uint8_t, maxFirmwareLength>{};
// NOLINTBEGIN(*reinterpret-cast)
Partition const primaryPartition{
    .startAddress = reinterpret_cast<std::uintptr_t>(primaryPartitionMemory.data()),
    .flashSector = 0};
Partition const secondaryPartition1{
    .startAddress = reinterpret_cast<std::uintptr_t>(secondaryPartition1Memory.data()),
    .flashSector = 1};
Partition const secondaryPartition2{
    .startAddress = reinterpret_cast<std::uintptr_t>(secondaryPartition2Memory.data()),
    .flashSector = 2};
// NOLINTEND(*reinterpret-cast)
#else
Partition const primaryPartition{.startAddress = 0x0802'0000U, .flashSector = FLASH_Sector_5};
Partition const secondaryPartition1{.startAddress = 0x0804'0000U, .flashSector = FLASH_Sector_6};
Partition const secondaryPartition2{.startAddress = 0x0806'0000U, .flashSector = FLASH_Sector_7};
#endif


// Partition structure:
// - Length: without CRC at the end, 4 bytes, in little endian
// - Data
// - Checksum: CRC-32 over length + data, in little endian
auto CheckFirmwareIntegrity(std::uintptr_t startAddress) -> Result<void>
{
    if((startAddress % sizeof(std::uint32_t)) != 0)
    {
        return ErrorCode::misaligned;
    }
    // NOLINTNEXTLINE(*reinterpret-cast, performance-no-int-to-ptr)
    auto length = *reinterpret_cast<std::uint32_t volatile *>(startAddress);
    // The length includes the 4 bytes for itself so it must be >= 4. Since the CRC-32 value at the
    // end of the FW image must be aligned to 4 bytes, the length must also be a multiple of 4.
    auto lengthIsValid = sizeof(std::uint32_t) <= length and length <= maxFirmwareLength
                     and (length % sizeof(std::uint32_t)) == 0;
    if(not lengthIsValid)
    {
        return ErrorCode::invalidLength;
    }
    auto buffer = etl::vector<Byte, 128>{};  // NOLINT(*magic-numbers)
    buffer.resize(sizeof(length));
    Read(startAddress, Span(&buffer));
    auto crc = ComputeCrc32(Span(buffer));
    auto lengthWithChecksum = length + sizeof(crc);
    for(auto i = sizeof(length); i < lengthWithChecksum;)
    {
        auto nBytes = std::min<std::size_t>(lengthWithChecksum - i, buffer.capacity());
        buffer.resize(nBytes);
        Read(startAddress + i, Span(&buffer));
        crc = ComputeCrc32(crc, Span(buffer));
        i += nBytes;
    }
    if(crc != 0)
    {
        return ErrorCode::corrupt;
    }
    return outcome_v2::success();
}


auto Erase(std::uint16_t flashSector) -> Result<void>
{
#ifdef __linux__
    static constexpr auto erasedByte = 0xFF;
    if(flashSector == primaryPartition.flashSector)
    {
        primaryPartitionMemory.fill(erasedByte);
    }
    else if(flashSector == secondaryPartition1.flashSector)
    {
        secondaryPartition1Memory.fill(erasedByte);
    }
    else if(flashSector == secondaryPartition2.flashSector)
    {
        secondaryPartition2Memory.fill(erasedByte);
    }
    else
    {
        return ErrorCode::eraseFailed;
    }
    return outcome_v2::success();
#else
    FLASH_Unlock();
    auto flashStatus = FLASH_EraseSector(flashSector, VoltageRange_3);
    FLASH_Lock();
    if(flashStatus != FLASH_COMPLETE)
    {
        return ErrorCode::eraseFailed;
    }
    return outcome_v2::success();
#endif
}


auto Program(std::uintptr_t address, std::span<Byte const> data) -> Result<std::uintptr_t>
{
#ifdef __linux__
    // NOLINTNEXTLINE(*reinterpret-cast, performance-no-int-to-ptr)
    std::ranges::copy(data, reinterpret_cast<Byte *>(address));
    return address + data.size();
#else
    FLASH_Unlock();
    auto result = [&]() -> Result<std::uintptr_t>
    {
        for(auto i = 0U; i < data.size(); ++i)
        {
            auto flashStatus = FLASH_ProgramByte(address + i, static_cast<std::uint8_t>(data[i]));
            if(flashStatus != FLASH_COMPLETE)
            {
                return ErrorCode::programFailed;
            }
        }
        return address + data.size();
    }();
    FLASH_Lock();
    return result;
#endif
}


auto Read(std::uintptr_t address, std::span<Byte> data) -> void
{
    // NOLINTNEXTLINE(*reinterpret-cast, performance-no-int-to-ptr)
    auto bytes = std::span(reinterpret_cast<Byte const volatile *>(address), data.size());
    std::ranges::copy(bytes, data.begin());
}
}
