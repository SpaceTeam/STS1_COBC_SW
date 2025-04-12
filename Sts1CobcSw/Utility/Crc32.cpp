#include <Sts1CobcSw/Utility/Crc32.hpp>

#include <littlefs/lfs_util.h>

#include <array>
#include <climits>
#include <type_traits>


namespace sts1cobcsw::utility
{
std::uint32_t const initialCrc32Value = 0xFFFFFFFFU;


//! @brief  Compute the CRC32 over a given data buffer in software using the
//! implementation of littlefs.
//!
//! @param  data    The data buffer
//! @return The corresponding CRC32 checksum
auto ComputeCrc32(std::span<Byte const> data) -> std::uint32_t
{
    return lfs_crc(initialCrc32Value, data.data(), data.size_bytes());
}


//! @brief  Compute the CRC32 over a given data buffer in software using the
//! implementation of littlefs. Allows the use of a custom initial value, if over multiple data
//! segments a CRC32 should be calculated.
//!
//! @param  initialValue    The value used to start the CRC
//! @param  data    The data buffer
//! @return The corresponding CRC32 checksum
auto ComputeCrc32(std::uint32_t initialValue, std::span<Byte const> data) -> std::uint32_t
{
    return lfs_crc(initialValue, data.data(), data.size_bytes());
}
}
