#include <Tests/CatchRodos/TestMacros.hpp>
#include <Tests/Utility/Stringification.hpp>  // IWYU pragma: keep

#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#ifndef __linux__
    #include <rodos/src/bare-metal/stm32f4/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_flash.h>
#endif

#include <etl/vector.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>


using sts1cobcsw::Byte;
using sts1cobcsw::CheckFirmwareIntegrity;
using sts1cobcsw::ErrorCode;

using sts1cobcsw::operator""_b;


#ifndef __linux__
auto ProgramFlash(std::span<Byte const> data, std::uintptr_t address) -> bool;
#endif


TEST_CASE("Firmware integrity check RAM")
{
    // clang-format off
    auto data = etl::vector<Byte, 20>{
        0x0C_b, 0x00_b, 0x00_b, 0x00_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b,
        0xAB_b, 0xFF_b, 0x10_b, 0x01_b};
    // clang-format on
    auto result = sts1cobcsw::ComputeCrc32(sts1cobcsw::Span(data));
    auto serializedCrc = sts1cobcsw::Serialize(result);
    data.insert(data.end(), serializedCrc.begin(), serializedCrc.end());

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto startAddress = reinterpret_cast<std::uintptr_t>(data.data());

    auto checkFirmwareResult = CheckFirmwareIntegrity(startAddress);
    CHECK(checkFirmwareResult.has_error() == false);

    // Content is modified -> CRC is not correct anymore
    data[4] ^= 0xFF_b;
    checkFirmwareResult = CheckFirmwareIntegrity(startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::corrupt);
    data[4] ^= 0xFF_b;  // Restore original content

    // Length must be a multiple of 4
    data[0] = 0x0B_b;
    checkFirmwareResult = CheckFirmwareIntegrity(startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);
    data[0] = 0x0A_b;
    checkFirmwareResult = CheckFirmwareIntegrity(startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);
    data[0] = 0x09_b;
    checkFirmwareResult = CheckFirmwareIntegrity(startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);

    // Length must be >= 4
    data[0] = 0x03_b;
    checkFirmwareResult = CheckFirmwareIntegrity(startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);

    // Length must be <= 0x20000
    data[0] = 0x04_b;
    data[1] = 0x00_b;
    data[2] = 0x02_b;
    data[3] = 0x00_b;
    checkFirmwareResult = CheckFirmwareIntegrity(startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);

    // Partition must be aligned to 4 bytes
    std::rotate(data.rbegin(), data.rbegin() + 1, data.rend());  // Rotate right
    checkFirmwareResult = CheckFirmwareIntegrity(startAddress + 1);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::misaligned);
}


#ifndef __linux__
// TODO: Do fewer tests because erasing and programming the flash is slow
TEST_CASE("Firmware integrity check flash")
{
    // clang-format off
    auto data = etl::vector<Byte, 20>{
        0x0C_b, 0x00_b, 0x00_b, 0x00_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b,
        0xAB_b, 0xFF_b, 0x10_b, 0x01_b};
    // clang-format on
    auto result = sts1cobcsw::ComputeCrc32(sts1cobcsw::Span(data));
    auto serializedCrc = sts1cobcsw::Serialize(result);
    data.insert(data.end(), serializedCrc.begin(), serializedCrc.end());

    for(auto partition : {sts1cobcsw::primaryPartitionStartAddress,
                          sts1cobcsw::secondaryPartition1StartAddress,
                          sts1cobcsw::secondaryPartition2StartAddress})
    {
        auto programmingFlashSucceeded = ProgramFlash(data, partition);
        REQUIRE(programmingFlashSucceeded);
        auto checkFirmwareResult = CheckFirmwareIntegrity(partition);
        CHECK(checkFirmwareResult.has_error() == false);
    }

    auto partition = sts1cobcsw::secondaryPartition1StartAddress;

    // Content is modified -> CRC is not correct anymore
    data[4] ^= 0xFF_b;
    auto programmingFlashSucceeded = ProgramFlash(data, partition);
    REQUIRE(programmingFlashSucceeded);
    auto checkFirmwareResult = CheckFirmwareIntegrity(partition);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::corrupt);
    data[4] ^= 0xFF_b;
    programmingFlashSucceeded = ProgramFlash(data, partition);
    REQUIRE(programmingFlashSucceeded);

    // Length must be a multiple of 4
    data[0] = 0x0B_b;
    programmingFlashSucceeded = ProgramFlash(data, partition);
    REQUIRE(programmingFlashSucceeded);
    checkFirmwareResult = CheckFirmwareIntegrity(partition);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);
    data[0] = 0x0A_b;
    programmingFlashSucceeded = ProgramFlash(data, partition);
    REQUIRE(programmingFlashSucceeded);
    checkFirmwareResult = CheckFirmwareIntegrity(partition);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);
    data[0] = 0x09_b;
    programmingFlashSucceeded = ProgramFlash(data, partition);
    REQUIRE(programmingFlashSucceeded);
    checkFirmwareResult = CheckFirmwareIntegrity(partition);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);

    // Length must be >= 4
    data[0] = 0x03_b;
    programmingFlashSucceeded = ProgramFlash(data, partition);
    REQUIRE(programmingFlashSucceeded);
    checkFirmwareResult = CheckFirmwareIntegrity(partition);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);

    // Length must be <= 0x20000
    data[0] = 0x04_b;
    data[1] = 0x00_b;
    data[2] = 0x02_b;
    data[3] = 0x00_b;
    programmingFlashSucceeded = ProgramFlash(data, partition);
    REQUIRE(programmingFlashSucceeded);
    checkFirmwareResult = CheckFirmwareIntegrity(partition);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);
}


auto ProgramFlash(std::span<Byte const> data, std::uintptr_t address) -> bool
{
    auto flashSector = 0U;
    switch(address)
    {
        case sts1cobcsw::primaryPartitionStartAddress:
            flashSector = FLASH_Sector_5;
            break;
        case sts1cobcsw::secondaryPartition1StartAddress:
            flashSector = FLASH_Sector_6;
            break;
        case sts1cobcsw::secondaryPartition2StartAddress:
            flashSector = FLASH_Sector_7;
            break;
        default:
            return false;
    }
    FLASH_Unlock();
    auto resultFlash = FLASH_EraseSector(flashSector, VoltageRange_3);
    if(resultFlash != FLASH_COMPLETE)
    {
        return false;
    }
    for(auto i = 0U; i < data.size(); ++i)
    {
        auto dataByte = static_cast<std::uint8_t>(data[i]);
        resultFlash = FLASH_ProgramByte(address + i, dataByte);
        if(resultFlash != FLASH_COMPLETE)
        {
            return false;
        }
    }
    FLASH_Lock();
    return true;
}
#endif
