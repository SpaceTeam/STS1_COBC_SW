#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduNames.hpp>

namespace sts1cobcsw::periphery
{
EduUartInterface::EduUartInterface()
{
    // EDU PDD agrees with the default baudrate (115200 bit/s)
    mEduUart_.init();
}

auto EduUartInterface::Crc32(std::span<uint8_t> data, size_t nBytes) -> uint32_t
{
    // From https://en.wikipedia.org/wiki/Cyclic_redundancy_check
    // Lookup table from https://gist.github.com/Miliox/b86b60b9755faf3bd7cf

    uint32_t crc32 = crc32Init;

    for(size_t i = 0; i < nBytes; i++)
    {
        const uint32_t lookupIndex = (crc32 ^ data[i]) & 0xFFU;
        crc32 =
            (crc32 >> 8U) ^ crcTable[lookupIndex];  // CRCTable is an array of 256 32-bit constants
    }

    // Finalize the CRC-32 value by inverting all the bits
    crc32 ^= 0xFFFFFFFFU;
    return crc32;
}

void EduUartInterface::SendData(std::span<uint8_t> data)
{
    hal::WriteTo(&mEduUart_, data);
    for(size_t i = 0; i < data.size(); i++)
    {
        PRINTF("data[%i] = %i\n", i, data[i]);
    }
}
}