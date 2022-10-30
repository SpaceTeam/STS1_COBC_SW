#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/UtilityNames.hpp>


namespace sts1cobcsw::utility
{
[[nodiscard]] auto Crc32(std::span<sts1cobcsw::serial::Byte> data) -> uint32_t
{
    std::uint32_t crc32 = crc32Init;
    std::size_t nBytes = data.size();

    for(std::size_t i = 0; i < nBytes; i++)
    {
        const std::uint32_t lookupIndex =
            ((crc32 >> 24U) ^ std::to_integer<std::uint32_t>(data[i])) & 0xFFU;
        crc32 = (crc32 << oneByteWidth)
              ^ crcTable[lookupIndex];  // CRCTable is an array of 256 32-bit constants
    }

    return crc32;
}
}
