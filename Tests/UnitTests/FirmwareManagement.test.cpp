#include <Tests/CatchRodos/TestMacros.hpp>
#include <Tests/Utility/Stringification.hpp>  // IWYU pragma: keep

#include <Sts1CobcSw/ErrorDetectionAndCorrection/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <etl/vector.h>

#include <algorithm>
#include <array>
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


TEST_CASE(
    "Firmware management: Erase(), Program(), Read(), ReadFirmwareLength(), and "
    "ReadFirmwareCommitHash()")
{
    auto eraseResult = fw::Erase(fw::primaryPartition.flashSector);
    REQUIRE(eraseResult.has_error() == false);
    eraseResult = fw::Erase(fw::secondaryPartition1.flashSector);
    REQUIRE(eraseResult.has_error() == false);
    eraseResult = fw::Erase(fw::secondaryPartition2.flashSector);
    REQUIRE(eraseResult.has_error() == false);
    auto data = std::array<Byte, 100>{};
    fw::Read(fw::primaryPartition.startAddress, std::span(data));
    CHECK(std::ranges::all_of(data, [](Byte byte) { return byte == 0xFF_b; }));
    fw::Read(fw::secondaryPartition1.startAddress, std::span(data));
    CHECK(std::ranges::all_of(data, [](Byte byte) { return byte == 0xFF_b; }));
    fw::Read(fw::secondaryPartition2.startAddress, std::span(data));
    CHECK(std::ranges::all_of(data, [](Byte byte) { return byte == 0xFF_b; }));

    data.fill(0x00_b);
    auto programResult = fw::Program(fw::primaryPartition.startAddress, std::span(data));
    REQUIRE(programResult.has_error() == false);
    programResult = fw::Program(fw::secondaryPartition1.startAddress, std::span(data));
    REQUIRE(programResult.has_error() == false);
    programResult = fw::Program(fw::secondaryPartition2.startAddress, std::span(data));
    REQUIRE(programResult.has_error() == false);
    fw::Read(fw::primaryPartition.startAddress, std::span(data));
    CHECK(std::ranges::all_of(data, [](Byte byte) { return byte == 0x00_b; }));
    fw::Read(fw::secondaryPartition1.startAddress, std::span(data));
    CHECK(std::ranges::all_of(data, [](Byte byte) { return byte == 0x00_b; }));
    fw::Read(fw::secondaryPartition2.startAddress, std::span(data));
    CHECK(std::ranges::all_of(data, [](Byte byte) { return byte == 0x00_b; }));

    auto length = 0x10U;
    auto lengthAddress = reinterpret_cast<std::uintptr_t>(&length);  // NOLINT(*reinterpret-cast)
    auto lengthResult = fw::ReadFirmwareLength(lengthAddress);
    REQUIRE(lengthResult.has_error() == false);
    CHECK(lengthResult.value() == length);

    length = sizeof(std::uint32_t) + 1;
    lengthResult = fw::ReadFirmwareLength(lengthAddress);
    REQUIRE(lengthResult.has_error() == true);
    CHECK(lengthResult.error() == ErrorCode::invalidLength);

    lengthResult = fw::ReadFirmwareLength(0x01);
    REQUIRE(lengthResult.has_error() == true);
    CHECK(lengthResult.error() == ErrorCode::misaligned);

    for(auto i = 0U; i < fw::commitHashLength; i++)
    {
        data[i] = static_cast<Byte>(i);
    }
    auto commitHash = etl::vector<Byte, fw::commitHashLength>{};
    commitHash.resize(fw::commitHashLength);
    auto hashAddress = reinterpret_cast<std::uintptr_t>(data.data());  // NOLINT(*reinterpret-cast)
    auto commitHashResult = fw::ReadFirmwareCommitHash(hashAddress, sts1cobcsw::Span(&commitHash));
    CHECK(commitHashResult.has_error() == false);
    for(auto i = 0U; i < fw::commitHashLength; i++)
    {
        data[i] = commitHash[i];
    }

    auto smallVector = etl::vector<Byte, 1>();
    commitHashResult = fw::ReadFirmwareCommitHash(hashAddress, sts1cobcsw::Span(&smallVector));
    REQUIRE(commitHashResult.has_error() == true);
    CHECK(commitHashResult.error() == ErrorCode::invalidLength);

    commitHashResult = fw::ReadFirmwareCommitHash(1, sts1cobcsw::Span(&commitHash));
    REQUIRE(commitHashResult.has_error() == true);
    CHECK(commitHashResult.error() == ErrorCode::misaligned);
}


TEST_CASE("Firmware management: CheckFirmwareIntegrity()")
{
    // clang-format off
    auto testFirmware = etl::vector<Byte, 20>{
        0x0C_b, 0x00_b, 0x00_b, 0x00_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b,
        0xAB_b, 0xFF_b, 0x10_b, 0x01_b};
    // clang-format on
    auto result = sts1cobcsw::ComputeCrc32(sts1cobcsw::Span(testFirmware));
    auto serializedCrc = sts1cobcsw::Serialize(result);
    testFirmware.insert(testFirmware.end(), serializedCrc.begin(), serializedCrc.end());

    for(auto partition : {fw::primaryPartition, fw::secondaryPartition1, fw::secondaryPartition2})
    {
        auto eraseAndProgramResult = EraseAndProgram(partition, testFirmware);
        REQUIRE(eraseAndProgramResult.has_error() == false);
        auto checkFirmwareResult = fw::CheckFirmwareIntegrity(partition.startAddress);
        CHECK(checkFirmwareResult.has_error() == false);
    }

    auto partition = fw::secondaryPartition1;

    // Content is modified -> CRC is not correct anymore
    testFirmware[4] ^= 0xFF_b;
    auto eraseAndProgramResult = EraseAndProgram(partition, testFirmware);
    REQUIRE(eraseAndProgramResult.has_error() == false);
    auto checkFirmwareResult = fw::CheckFirmwareIntegrity(partition.startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::corrupt);
    testFirmware[4] ^= 0xFF_b;
    eraseAndProgramResult = EraseAndProgram(partition, testFirmware);
    REQUIRE(eraseAndProgramResult.has_error() == false);

    // Length must be a multiple of 4
    testFirmware[0] = 0x0B_b;
    eraseAndProgramResult = EraseAndProgram(partition, testFirmware);
    REQUIRE(eraseAndProgramResult.has_error() == false);
    checkFirmwareResult = fw::CheckFirmwareIntegrity(partition.startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);
    testFirmware[0] = 0x0A_b;
    eraseAndProgramResult = EraseAndProgram(partition, testFirmware);
    REQUIRE(eraseAndProgramResult.has_error() == false);
    checkFirmwareResult = fw::CheckFirmwareIntegrity(partition.startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);
    testFirmware[0] = 0x09_b;
    eraseAndProgramResult = EraseAndProgram(partition, testFirmware);
    REQUIRE(eraseAndProgramResult.has_error() == false);
    checkFirmwareResult = fw::CheckFirmwareIntegrity(partition.startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);

    // Length must be >= 4
    testFirmware[0] = 0x03_b;
    eraseAndProgramResult = EraseAndProgram(partition, testFirmware);
    REQUIRE(eraseAndProgramResult.has_error() == false);
    checkFirmwareResult = fw::CheckFirmwareIntegrity(partition.startAddress);
    CHECK(checkFirmwareResult.has_error());
    CHECK(checkFirmwareResult.error() == ErrorCode::invalidLength);

    // Length must be <= 0x20000
    testFirmware[0] = 0x04_b;
    testFirmware[1] = 0x00_b;
    testFirmware[2] = 0x02_b;
    testFirmware[3] = 0x00_b;
    eraseAndProgramResult = EraseAndProgram(partition, testFirmware);
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
