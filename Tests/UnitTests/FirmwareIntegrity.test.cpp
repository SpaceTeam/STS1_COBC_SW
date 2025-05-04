#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/FirmwareIntegrity.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#ifndef __linux__
    #include <rodos/src/bare-metal/stm32f4/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_flash.h>
#endif

#include <etl/vector.h>

#include <algorithm>
#include <cstdint>


namespace sts1cobcsw
{
using sts1cobcsw::operator""_b;

#ifndef __linux__
auto ProgramFlash(std::span<const sts1cobcsw::Byte> data, utility::Partition partition) -> void;
#endif

TEST_CASE("Firmware Integrity Ram")
{
    // Length: 11 = 0x0000000B -> 0x0B000000 (Little endian)
    auto partition = etl::vector<Byte, 20>{
        0x0B_b, 0x00_b, 0x00_b, 0x00_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b, 0xAB_b, 0xFF_b, 0x10_b};

    auto result = utility::ComputeCrc32(Span(partition));
    auto serializedCrc = Serialize(result);
    partition.insert(partition.end(), serializedCrc.begin(), serializedCrc.end());

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto ramPartitionStartAddress = reinterpret_cast<std::uintptr_t>(partition.data());
    auto ramPartitionEnum = static_cast<utility::Partition>(ramPartitionStartAddress);
    auto checkFirmwareResult = utility::CheckFirmwareIntegrity(ramPartitionEnum);
    CHECK(checkFirmwareResult.has_error() == false);

    // Content is modified -> CRC is not correct anymore
    partition[4] = 0xFF_b;
    checkFirmwareResult = utility::CheckFirmwareIntegrity(ramPartitionEnum);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::corrupt);

    // When the length of the partition is empty, an error is returned
    partition[0] = 0x00_b;
    checkFirmwareResult = utility::CheckFirmwareIntegrity(ramPartitionEnum);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::empty);

    // Sector length 0x000000FF -> 0xFF000000 which is larger than allowed (0x20000ULL)
    partition[3] = 0xFF_b;
    checkFirmwareResult = utility::CheckFirmwareIntegrity(ramPartitionEnum);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);
}


#ifndef __linux__
TEST_CASE("Firmware Integrity Flash")
{
    // Length: 11 = 0x0000000B -> 0x0B000000 (Little endian)
    auto partition = etl::vector<Byte, 20>{
        0x0B_b, 0x00_b, 0x00_b, 0x00_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b, 0xAB_b, 0xFF_b, 0x10_b};

    auto result = utility::ComputeCrc32(Span(partition));
    auto serializedCrc = Serialize(result);
    partition.insert(partition.end(), serializedCrc.begin(), serializedCrc.end());

    auto partitionEnum = utility::Partition::secondary1;

    // Write data to Flash
    ProgramFlash(partition, partitionEnum);
    auto checkFirmwareResult = utility::CheckFirmwareIntegrity(partitionEnum);
    CHECK(checkFirmwareResult.has_error() == false);

    // Content is modified -> CRC is not correct anymore
    partition[4] = 0xFF_b;
    ProgramFlash(partition, partitionEnum);
    checkFirmwareResult = utility::CheckFirmwareIntegrity(partitionEnum);
    CHECK(checkFirmwareResult.has_error() == true);
    // CHECK(checkFirmwareResult.error() == ErrorCode::corrupt);

    // When the length of the partition is empty, an error is returned
    partition[0] = 0x00_b;
    ProgramFlash(partition, partitionEnum);
    checkFirmwareResult = utility::CheckFirmwareIntegrity(partitionEnum);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::empty);

    // Sector length 0x000000FF -> 0xFF000000 which is larger than allowed (0x20000ULL)
    partition[3] = 0xFF_b;
    ProgramFlash(partition, partitionEnum);
    checkFirmwareResult = utility::CheckFirmwareIntegrity(partitionEnum);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);
}


auto ProgramFlash(std::span<const sts1cobcsw::Byte> data, utility::Partition partition) -> void
{
    auto flashSector = 0U;
    switch(partition)
    {
        case utility::Partition::secondary1:
            flashSector = FLASH_Sector_6;
            break;
        case utility::Partition::secondary2:
            flashSector = FLASH_Sector_7;
            break;
        default:
            CHECK(false);
            return;  // Don't overwrite own code
    }
    auto partitionAddress = static_cast<std::uint32_t>(partition);

    // Write data to Flash
    FLASH_Unlock();
    auto resultFlash = FLASH_EraseSector(flashSector, VoltageRange_3);
    CHECK(resultFlash == FLASH_COMPLETE);
    for(std::uint32_t offset = 0; offset < data.size(); offset++)
    {
        auto address = static_cast<std::uint32_t>(partitionAddress + offset);
        auto dataByte = static_cast<std::uint8_t>(data[offset]);
        resultFlash = FLASH_ProgramByte(address, dataByte);
        CHECK(resultFlash == FLASH_COMPLETE);
    }
    FLASH_Lock();
}
#endif
}
