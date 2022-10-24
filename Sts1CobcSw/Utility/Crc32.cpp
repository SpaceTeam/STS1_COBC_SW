#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/UtilityNames.hpp>


namespace sts1cobcsw::utility
{
using sts1cobcsw::serial::Byte;


[[nodiscard]] auto Crc32(std::span<Byte> data) -> uint32_t
{
    uint32_t crc32 = crc32Init;
    size_t nBytes = data.size();

    for(size_t i = 0; i < nBytes; i++)
    {
        const uint32_t lookupIndex = ((crc32 >> 24U) ^ std::to_integer<uint32_t>(data[i])) & 0xFFU;
        crc32 = (crc32 << oneByteWidth)
              ^ crcTable[lookupIndex];  // CRCTable is an array of 256 32-bit constants
    }

    return crc32;
}
}
