#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>

#include <Sts1CobcSw/ErrorDetectionAndCorrection/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <etl/vector.h>

#ifndef __linux__
    #include <Sts1CobcSw/Bootloader/stm32f411xe.h>
#endif

#include <algorithm>
#ifdef __linux__
    #include <array>
#endif
#include <cstddef>
#include <span>


namespace sts1cobcsw::fw
{
constexpr auto maxFirmwareLength = 0x2'0000U;

namespace
{
auto const flashSector5 = (static_cast<uint16_t>(0x0028));
auto const flashSector6 = (static_cast<uint16_t>(0x0030));
auto const flashSector7 = (static_cast<uint16_t>(0x0038));
auto const key1 = (static_cast<uint32_t>(0x4567'0123));
auto const key2 = (static_cast<uint32_t>(0xCDEF'89AB));


auto UnlockFlash() -> void;
auto LockFlash() -> void;
auto ProgramFlashByte(std::uintptr_t address, std::uint8_t data) -> bool;
auto EraseFlashSector(std::uint16_t sector) -> bool;
}

#ifdef __linux__
namespace
{
auto primaryPartitionMemory = std::array<std::uint8_t, maxFirmwareLength>{};
auto secondaryPartition1Memory = std::array<std::uint8_t, maxFirmwareLength>{};
auto secondaryPartition2Memory = std::array<std::uint8_t, maxFirmwareLength>{};
}
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
Partition const primaryPartition{.startAddress = 0x0802'0000U, .flashSector = flashSector5};
Partition const secondaryPartition1{.startAddress = 0x0804'0000U, .flashSector = flashSector6};
Partition const secondaryPartition2{.startAddress = 0x0806'0000U, .flashSector = flashSector7};
#endif


auto GetPartition(PartitionId partitionId) -> Result<Partition>
{
    switch(partitionId)
    {
        case PartitionId::primary:
            return primaryPartition;
        case PartitionId::secondary1:
            return secondaryPartition1;
        case PartitionId::secondary2:
            return secondaryPartition2;
    }
    return ErrorCode::invalidPartitionId;
}


// Partition structure:
// - Length: without CRC at the end, 4 bytes, in little endian
// - Data
// - Checksum: CRC-32 over length + data, in little endian
auto GetCrcs(std::uintptr_t startAddress) -> Crcs
{
    Crcs crcs;
    if((startAddress % sizeof(std::uint32_t)) != 0)
    {
        crcs.aligned = false;
        return crcs;
    }
    // NOLINTNEXTLINE(*reinterpret-cast, performance-no-int-to-ptr)
    auto length = *reinterpret_cast<std::uint32_t volatile *>(startAddress);
    // The length includes the 4 bytes for itself so it must be >= 4. Since the CRC-32 value at the
    // end of the FW image must be aligned to 4 bytes, the length must also be a multiple of 4.
    auto lengthIsValid = sizeof(std::uint32_t) <= length and length <= maxFirmwareLength
                     and (length % sizeof(std::uint32_t)) == 0;
    if(not lengthIsValid)
    {
        crcs.validLength = false;
        return crcs;
    }
    auto buffer = etl::vector<Byte, 128>{};  // NOLINT(*magic-numbers)
    buffer.resize(sizeof(length));
    Read(startAddress, Span(&buffer));
    auto newCrc = ComputeCrc32(Span(buffer));
    buffer.resize(4);
    Read(startAddress + length, Span(&buffer));
// NOLINTBEGIN(*magic-numbers, readability-magic-numbers)
#pragma GCC diagnostic push
    auto oldCrc = static_cast<std::uint32_t>(buffer[0])
                | (static_cast<std::uint32_t>(buffer[1]) << 8U)
                | (static_cast<std::uint32_t>(buffer[2]) << 16U)
                | (static_cast<std::uint32_t>(buffer[3]) << 24U);
#pragma GCC diagnostic pop
    // NOLINTEND(*magic-numbers, readability-magic-numbers)
    return Crcs{.validLength = true, .aligned = true, .newCheckSum = newCrc, .oldCheckSum = oldCrc};
}


auto CheckFirmwareIntegrity(std::uintptr_t startAddress) -> Result<void>
{
    auto crcs = GetCrcs(startAddress);
    if(not crcs.aligned)
    {
        return ErrorCode::misaligned;
    }
    if(not crcs.validLength)
    {
        return ErrorCode::invalidLength;
    }
    if(crcs.newCheckSum != crcs.oldCheckSum)
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
    UnlockFlash();
    auto flashStatus = EraseFlashSector(flashSector);
    LockFlash();
    if(!flashStatus)
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
    UnlockFlash();
    auto result = [&]() -> Result<std::uintptr_t>
    {
        for(auto i = 0U; i < data.size(); ++i)
        {
            auto flashStatus = ProgramFlashByte(address + i, static_cast<std::uint8_t>(data[i]));
            if(!flashStatus)
            {
                return ErrorCode::programFailed;
            }
        }
        return address + data.size();
    }();
    LockFlash();
    return result;
#endif
}


auto Read(std::uintptr_t address, std::span<Byte> data) -> void
{
    // NOLINTNEXTLINE(*reinterpret-cast, performance-no-int-to-ptr)
    auto bytes = std::span(reinterpret_cast<Byte const volatile *>(address), data.size());
    std::ranges::copy(bytes, data.begin());
}


namespace
{
// NOLINTBEGIN(*no-int-to-ptr, *cstyle-cast)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

auto UnlockFlash() -> void
{
    if((FLASH->CR & FLASH_CR_LOCK) != 0)
    {
        // Authorize the FLASH Registers access
        FLASH->KEYR = key1;
        FLASH->KEYR = key2;
    }
}

auto LockFlash() -> void
{
    FLASH->CR |= FLASH_CR_LOCK;
}


auto ProgramFlashByte(std::uintptr_t address, std::uint8_t data) -> bool
{
    // Wait for last operation to be completed
    while(FLASH->SR & FLASH_SR_BSY) {}

    FLASH->CR &= ~FLASH_CR_PSIZE;
    auto const psizeShift = static_cast<std::uint32_t>(8U);
    FLASH->CR |= (0x0U << psizeShift);
    FLASH->CR |= FLASH_CR_PG;

    // NOLINTNEXTLINE(*reinterpret-cast, performance-no-int-to-ptr)
    *(reinterpret_cast<uint8_t volatile *>(address)) = data;

    // Wait for last operation to be completed
    while(FLASH->SR & FLASH_SR_BSY) {}

    // check for errors
    if(FLASH->SR & (FLASH_SR_PGSERR | FLASH_SR_PGPERR | FLASH_SR_PGAERR | FLASH_SR_WRPERR))
    {
        // Clear error flags
        FLASH->SR |= FLASH_SR_PGSERR | FLASH_SR_PGPERR | FLASH_SR_PGAERR | FLASH_SR_WRPERR;
        // disable the PG bit
        FLASH->CR &= (~FLASH_CR_PG);
        return false;
    }
    // disable the PG bit
    FLASH->CR &= (~FLASH_CR_PG);
    return true;
}

auto EraseFlashSector(std::uint16_t sector) -> bool
{
    int const numberOfSectors = 12;
    if(sector > numberOfSectors - 1)
    {
        return false;
    }

    // Wait for last operation to be completed
    while(FLASH->SR & FLASH_SR_BSY) {}

    FLASH->CR &= ~FLASH_CR_PSIZE;
    FLASH->CR |= FLASH_CR_PSIZE_1;  // voltageRange
    FLASH->CR &= ~FLASH_CR_SNB;
    FLASH->CR |= FLASH_CR_SER | (static_cast<std::uint32_t>(sector) << FLASH_CR_SNB_Pos);
    FLASH->CR |= FLASH_CR_STRT;

    // Wait for last operation to be completed
    while(FLASH->SR & FLASH_SR_BSY) {}

    if(FLASH->SR & (FLASH_SR_PGSERR | FLASH_SR_PGPERR | FLASH_SR_PGAERR | FLASH_SR_WRPERR))
    {
        // Clear error flags
        FLASH->SR |= FLASH_SR_PGSERR | FLASH_SR_PGPERR | FLASH_SR_PGAERR | FLASH_SR_WRPERR;

        // disable the sector erase
        FLASH->CR &= (~FLASH_CR_SER);

        return false;
    }

    // disable sector erase
    FLASH->CR &= (~FLASH_CR_SER);

    // Return the Erase Status
    return true;
}

#pragma GCC diagnostic pop
// NOLINTEND(*no-int-to-ptr, *cstyle-cast)
}
}
