#include <Sts1CobcSw/Util/Util.hpp>
#include <Sts1CobcSw/Util/UtilNames.hpp>

#include <iostream>
#include <vector>

namespace sts1cobcsw::util
{
auto BytesToUint16(uint8_t msb, uint8_t lsb) -> uint16_t
{
    uint16_t retVal = 0U | static_cast<uint16_t>(msb << oneByteWidth) | lsb;
    return retVal;
}

auto BytesToUint32(uint8_t firstByte, uint8_t secondByte, uint8_t thirdByte, uint8_t fourthByte)
    -> uint32_t
{
    uint32_t retVal = 0U | static_cast<uint16_t>(firstByte << threeBytesWidth)
                    | static_cast<uint16_t>(secondByte << twoBytesWidth)
                    | static_cast<uint16_t>(thirdByte << oneByteWidth) | fourthByte;
    return retVal;
}

auto VecUint32ToUint8(std::span<uint32_t> src) -> std::vector<uint8_t>
{
    size_t srcSize = src.size();

    std::vector<uint8_t> dest(sizeof(uint32_t) * srcSize);

    for(size_t i = 0; i < dest.size(); i++)
    {
        dest[sizeof(uint32_t) * i] = (src[i] & uint32FirstByteMask) >> threeBytesWidth;
        dest[sizeof(uint32_t) * i + 1] = (src[i] & uint32SecondByteMask) >> twoBytesWidth;
        dest[sizeof(uint32_t) * i + 2] = (src[i] & uint32ThirdByteMask) >> oneByteWidth;
        dest[sizeof(uint32_t) * i + 3] = (src[i] & uint32FourthByteMask);
    }

    return dest;
}

auto VecUint16ToUint8(std::span<uint16_t> src) -> std::vector<uint8_t>
{
    size_t srcSize = src.size();

    std::vector<uint8_t> dest(sizeof(uint16_t) * srcSize);

    for(size_t i = 0; i < dest.size(); i++)
    {
        dest[sizeof(uint16_t) * i] = (src[i] & uint16FirstByteMask) >> oneByteWidth;
        dest[sizeof(uint16_t) * i + 1] = (src[i] & uint16SecondByteMask);
    }

    return dest;
}

auto Crc32(std::span<uint8_t> data) -> uint32_t
{
    uint32_t crc32 = crc32Init;
    size_t nBytes = data.size();

    for(size_t i = 0; i < nBytes; i++)
    {
        const uint32_t lookupIndex = ((crc32 >> 24U) ^ data[i]) & 0xFFU;
        crc32 = (crc32 << oneByteWidth)
              ^ crcTable[lookupIndex];  // CRCTable is an array of 256 32-bit constants
    }

    return crc32;
}
}
