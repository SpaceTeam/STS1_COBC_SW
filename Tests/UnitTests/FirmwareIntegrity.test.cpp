#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/FirmwareIntegrity.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <etl/vector.h>

#include <algorithm>
#include <cstdint>


namespace sts1cobcsw
{
using sts1cobcsw::operator""_b;


TEST_CASE("Firmware Integrity")
{
    // Partition structure:
    // First 4 Bytes length without crc at the end in little endian
    // Partition data
    // 4 Byte crc over length and partition data with initial Value 0x00 in little endian

    // Length: 11 = 0x0000000B -> 0x0B000000 (Little endian)
    auto partition = etl::vector<Byte, 20>{
        0x0B_b, 0x00_b, 0x00_b, 0x00_b, 0xCA_b, 0xBB_b, 0xA5_b, 0xE3_b, 0xAB_b, 0xFF_b, 0x10_b};

    // Compute Crc with start value 0x00, as the partition crc is calculated with that value
    auto result = utility::ComputeCrc32(0x00U, Span(partition));
    auto serializedCrc = Serialize(result);
    // Add crc32 in little endian format
    partition.insert(partition.end(), serializedCrc.begin(), serializedCrc.end());

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto ramPartitionStartAddress = reinterpret_cast<std::uintptr_t>(partition.data());
    auto ramPartitionEnum = static_cast<utility::Partition>(ramPartitionStartAddress);

    auto checkFirmwareResult = utility::CheckFirmwareIntegrity(ramPartitionEnum);
    CHECK(checkFirmwareResult.has_error() == false);

    // Content is modified -> crc is not correct anymore
    partition[4] = 0xFF_b;
    checkFirmwareResult = utility::CheckFirmwareIntegrity(ramPartitionEnum);
    CHECK(checkFirmwareResult.has_error() == true);
    CHECK(checkFirmwareResult.error() == ErrorCode::corrupt);

    // When the length of the partition is empty, a error is returned
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
}
