#include <Sts1CobcSw/Util/Util.hpp>
#include <Sts1CobcSw/Util/UtilNames.hpp>

#include <span>


namespace sts1cobcsw::utility
{
auto Crc32(std::span<uint8_t> data) -> uint32_t
{
    uint32_t crc32 = crc32Init;
    size_t nBytes = data.size();

    for(size_t i = 0; i < nBytes; i++)
    {
        const uint32_t lookupIndex = ((crc32 >> 24U) ^ data[i]) & 0xFFU;
        crc32 =
            (crc32 << 8U) ^ crcTable[lookupIndex];  // CRCTable is an array of 256 32-bit constants
    }

    return crc32;
}
}
