#include <Sts1CobcSw/ErrorDetectionAndCorrection/ErrorDetectionAndCorrection.hpp>

#include <littlefs/lfs_util.h>


namespace sts1cobcsw
{
constexpr std::uint32_t initialCrc32JamCrcValue = 0xFFFF'FFFF;


// Computes the CRC-32/JAMCRC (see
// https://crccalc.com/?crc=DEADBEEFABBA0102&method=CRC-32&datatype=hex&outtype=hex for comparisons)
auto ComputeCrc32(std::span<Byte const> data) -> std::uint32_t
{
    return lfs_crc(initialCrc32JamCrcValue, data.data(), data.size_bytes());
}


// Allows chaining CRC-32 computations
auto ComputeCrc32(std::uint32_t previousCrc32, std::span<Byte const> data) -> std::uint32_t
{
    return lfs_crc(previousCrc32, data.data(), data.size_bytes());
}
}
