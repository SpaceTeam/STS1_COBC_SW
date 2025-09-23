#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>

#ifndef __linux__
    #include <Sts1CobcSw/CmsisDevice/stm32f411xe.h>
#endif
#include <Sts1CobcSw/ErrorDetectionAndCorrection/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <etl/vector.h>

#include <algorithm>
#ifdef __linux__
    #include <array>
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::fw
{
namespace
{
#ifndef __linux__
auto UnlockFlash() -> void;
auto LockFlash() -> void;
auto Program(std::uintptr_t address, Byte data) -> bool;
auto DoErase(std::uint16_t sector) -> bool;
#endif


}


#ifdef __linux__
namespace
{
auto primaryPartitionMemory = std::array<std::uint8_t, partitionSize>{};
auto secondaryPartition1Memory = std::array<std::uint8_t, partitionSize>{};
auto secondaryPartition2Memory = std::array<std::uint8_t, partitionSize>{};
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
Partition const primaryPartition{.startAddress = 0x0802'0000U, .flashSector = 5};
Partition const secondaryPartition1{.startAddress = 0x0804'0000U, .flashSector = 6};
Partition const secondaryPartition2{.startAddress = 0x0806'0000U, .flashSector = 7};
#endif


auto GetPartition(PartitionId partitionId) -> Partition
{
    assert(partitionId == PartitionId::primary or partitionId == PartitionId::secondary1
           or partitionId == PartitionId::secondary2);
    switch(partitionId)
    {
        case PartitionId::primary:
            return primaryPartition;
        case PartitionId::secondary1:
            return secondaryPartition1;
        case PartitionId::secondary2:
            return secondaryPartition2;
    }
    return secondaryPartition1;
}


#ifndef BUILD_BOOTLOADER
auto CheckFirmwareIntegrity(std::uintptr_t startAddress) -> Result<void>
{
    // We don't have (want) ErrorCode::noError, so we use eduIsNotAlive which is obviously not an
    // error code that ComputeAndReadFirmwareChecksums() would return
    auto errorCode = ErrorCode::eduIsNotAlive;
    auto checksums = ComputeAndReadFirmwareChecksums(startAddress, &errorCode);
    if(errorCode != ErrorCode::eduIsNotAlive)
    {
        return errorCode;
    }
    if(checksums.computed != checksums.stored)
    {
        return ErrorCode::corrupt;
    }
    return outcome_v2::success();
}
#endif


// Partition structure:
// - Length: without CRC at the end, 4 bytes, in little endian
// - Data
// - Checksum: CRC-32 over length + data, in little endian
auto ComputeAndReadFirmwareChecksums(std::uintptr_t startAddress, ErrorCode * errorCode)
    -> FirmwareChecksums
{
    if((startAddress % sizeof(std::uint32_t)) != 0)
    {
        *errorCode = ErrorCode::misaligned;
        return {.computed = 0U, .stored = ~0U};
    }
    auto uint32Buffer = SerialBuffer<std::uint32_t>{};
    Read(startAddress, Span(&uint32Buffer));
    auto length = Deserialize<std::endian::little, std::uint32_t>(uint32Buffer);
    // The length includes the 4 bytes for itself but not the the CRC-32 checksum at the end. The
    // CRC-32 value must be 4-byte aligned.
    auto lengthIsValid = sizeof(std::uint32_t) <= length
                     and length <= (partitionSize - sizeof(std::uint32_t))
                     and (length % sizeof(std::uint32_t)) == 0;
    if(not lengthIsValid)
    {
        *errorCode = ErrorCode::invalidLength;
        return {.computed = 0U, .stored = ~0U};
    }

    auto buffer = etl::vector<Byte, 128>{};  // NOLINT(*magic-numbers)
    buffer.resize(sizeof(length));
    Read(startAddress, Span(&buffer));
    auto computedCrc = ComputeCrc32(Span(buffer));
    for(auto offset = sizeof(length); offset < length;)
    {
        auto chunkSize = std::min<std::size_t>(buffer.max_size(), length - offset);
        buffer.resize(chunkSize);
        Read(startAddress + offset, Span(&buffer));
        computedCrc = ComputeCrc32(computedCrc, Span(buffer));
        offset += chunkSize;
    }
    Read(startAddress + length, Span(&uint32Buffer));
    auto storedCrc = Deserialize<std::endian::little, std::uint32_t>(uint32Buffer);
    return FirmwareChecksums{.computed = computedCrc, .stored = storedCrc};
}


auto Erase(std::uint16_t flashSector) -> EraseResult
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
    auto eraseWasSuccessful = DoErase(flashSector);
    LockFlash();
    #ifdef BUILD_BOOTLOADER
    return eraseWasSuccessful;
    #else
    if(not eraseWasSuccessful)
    {
        return ErrorCode::eraseFailed;
    }
    return outcome_v2::success();
    #endif
#endif
}


auto Program(std::uintptr_t address, std::span<Byte const> data) -> ProgramResult
{
#ifdef __linux__
    // NOLINTNEXTLINE(*reinterpret-cast, performance-no-int-to-ptr)
    std::ranges::copy(data, reinterpret_cast<Byte *>(address));
    return address + data.size();
#else
    UnlockFlash();
    auto result = [&]() -> ProgramResult
    {
        for(auto i = 0U; i < data.size(); ++i)
        {
            auto flashStatus = Program(address + i, data[i]);
            if(!flashStatus)
            {
    #ifdef BUILD_BOOTLOADER
                return std::numeric_limits<std::uintptr_t>::max();
    #else
                return ErrorCode::programFailed;
    #endif
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

#ifndef __linux__
auto UnlockFlash() -> void
{
    static constexpr std::uint32_t key1 = 0x4567'0123;
    static constexpr std::uint32_t key2 = 0xCDEF'89AB;
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


auto Program(std::uintptr_t address, Byte data) -> bool
{
    // Wait for last operation to be completed
    while(FLASH->SR & FLASH_SR_BSY) {}

    FLASH->CR &= ~FLASH_CR_PSIZE;
    auto const psizeShift = static_cast<std::uint32_t>(8U);
    FLASH->CR |= (0x0U << psizeShift);
    FLASH->CR |= FLASH_CR_PG;

    // NOLINTNEXTLINE(*reinterpret-cast, performance-no-int-to-ptr)
    *(reinterpret_cast<Byte volatile *>(address)) = data;

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


auto DoErase(std::uint16_t sector) -> bool
{
    [[maybe_unused]] static constexpr auto nSectors = 12;
    assert(sector < nSectors);
    // Wait for last operation to be completed
    while(FLASH->SR & FLASH_SR_BSY) {}

    FLASH->CR &= ~FLASH_CR_PSIZE;
    FLASH->CR |= FLASH_CR_PSIZE_1;  // Voltage range
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
#endif

#pragma GCC diagnostic pop
// NOLINTEND(*no-int-to-ptr, *cstyle-cast)
}
}
