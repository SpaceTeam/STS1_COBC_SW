#include <Tests/CatchRodos/TestMacros.hpp>
#include <Tests/Utility/Stringification.hpp>  // IWYU pragma: keep

#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <etl/vector.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>


namespace fw = sts1cobcsw::fw;

using sts1cobcsw::Byte;
using sts1cobcsw::ErrorCode;

using sts1cobcsw::operator""_b;


namespace
{
auto EraseAndProgram(fw::Partition const & partition, std::span<Byte const> data)
    -> sts1cobcsw::Result<std::uintptr_t>;
}


// TODO: Add tests for Erase() and Program()
TEST_CASE("Firmware integrity check")
{
    // clang-format off
    auto data = etl::vector<Byte, 20>{
        0x0C_b, 0x00_b, 0x00_b, 0x00_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b,
        0xAB_b, 0xFF_b, 0x10_b, 0x01_b};
    // clang-format on
    auto result = sts1cobcsw::ComputeCrc32(sts1cobcsw::Span(data));
    auto serializedCrc = sts1cobcsw::Serialize(result);
    data.insert(data.end(), serializedCrc.begin(), serializedCrc.end());

    for(auto partition : {fw::primaryPartition, fw::secondaryPartition1, fw::secondaryPartition2})
    {
        auto eraseAndProgramResult = EraseAndProgram(partition, data);
        REQUIRE(eraseAndProgramResult.has_error() == false);
        auto checkFirmwareResult = fw::CheckFirmwareIntegrity(partition.startAddress);
        CHECK(checkFirmwareResult.has_error() == false);
    }

    auto partition = fw::secondaryPartition1;

    // Content is modified -> CRC is not correct anymore
    data[4] ^= 0xFF_b;
    auto eraseAndProgramResult = EraseAndProgram(partition, data);
    REQUIRE(eraseAndProgramResult.has_error() == false);
    auto checkFirmwareResult = fw::CheckFirmwareIntegrity(partition.startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::corrupt);
    data[4] ^= 0xFF_b;
    eraseAndProgramResult = EraseAndProgram(partition, data);
    REQUIRE(eraseAndProgramResult.has_error() == false);

    // Length must be a multiple of 4
    data[0] = 0x0B_b;
    eraseAndProgramResult = EraseAndProgram(partition, data);
    REQUIRE(eraseAndProgramResult.has_error() == false);
    checkFirmwareResult = fw::CheckFirmwareIntegrity(partition.startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);
    data[0] = 0x0A_b;
    eraseAndProgramResult = EraseAndProgram(partition, data);
    REQUIRE(eraseAndProgramResult.has_error() == false);
    checkFirmwareResult = fw::CheckFirmwareIntegrity(partition.startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);
    data[0] = 0x09_b;
    eraseAndProgramResult = EraseAndProgram(partition, data);
    REQUIRE(eraseAndProgramResult.has_error() == false);
    checkFirmwareResult = fw::CheckFirmwareIntegrity(partition.startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);

    // Length must be >= 4
    data[0] = 0x03_b;
    eraseAndProgramResult = EraseAndProgram(partition, data);
    REQUIRE(eraseAndProgramResult.has_error() == false);
    checkFirmwareResult = fw::CheckFirmwareIntegrity(partition.startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);

    // Length must be <= 0x20000
    data[0] = 0x04_b;
    data[1] = 0x00_b;
    data[2] = 0x02_b;
    data[3] = 0x00_b;
    eraseAndProgramResult = EraseAndProgram(partition, data);
    REQUIRE(eraseAndProgramResult.has_error() == false);
    checkFirmwareResult = fw::CheckFirmwareIntegrity(partition.startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);
}


namespace
{
auto EraseAndProgram(fw::Partition const & partition, std::span<Byte const> data)
    -> sts1cobcsw::Result<std::uintptr_t>
{
    OUTCOME_TRY(fw::Erase(partition.flashSector));
    return fw::Program(partition.startAddress, data);
}
}
