#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduNames.hpp>

#include <array>

namespace sts1cobcsw::periphery
{

// TODO: how can auto use arrays for src and dest but I can't?
auto ArrayUint16toUint8(auto & src, auto & dest) -> int32_t
{
    if(std::size(src) * 2 != std::size(dest) or std::size(src) == 0)
    {
        //PRINTF("Can't convert from uint16_t to uint8_t: mismatched array sizes!\n");
        return -1;
    }
    if(sizeof(src[0]) != 2 or sizeof(dest[0]) != 1)
    {
        //PRINTF("Can't convert from uint16_t to uint8_t: wrong types!\n");
        return -1;
    }

    // TODO: looks ugly
    auto rawData = reinterpret_cast<uint8_t *>(&src);
    for(size_t i = 0; i < std::size(dest); i++)
    {
        dest[i] = rawData[i];
    }
    return 0;
}

auto Crc32(std::span<uint8_t> data) -> uint32_t
{
    // From https://en.wikipedia.org/wiki/Cyclic_redundancy_check
    // Lookup table from https://gist.github.com/Miliox/b86b60b9755faf3bd7cf

    uint32_t crc32 = crc32Init;
    size_t nBytes = data.size();

    for(size_t i = 0; i < nBytes; i++)
    {
        const uint32_t lookupIndex = (crc32 ^ data[i]) & 0xFFU;
        crc32 =
            (crc32 >> 8U) ^ crcTable[lookupIndex];  // CRCTable is an array of 256 32-bit constants
    }

    // Finalize the CRC-32 value by inverting all the bits
    crc32 ^= crc32Init;
    return crc32;
}


EduUartInterface::EduUartInterface()
{
    // EDU PDD agrees with the default baudrate (115200 bit/s)
    // mEduUart_.init();
}

void EduUartInterface::SendCommand(uint8_t cmd)
{
    std::array<uint8_t, 1> cmdArr{cmd};
    // hal::WriteTo(&mEduUart_, cmdArr);
}

void EduUartInterface::SendData(std::span<uint8_t> data)
{
    size_t nBytes = data.size();
    if(nBytes >= dataMaxLength)
    {
        PRINTF("Could not send data packet: data field too long!\n");
        return;
    }
    for(size_t i = 0; i < nBytes; i++)
    {
        PRINTF("data[%i] = %i\n", i, data[i]);
    }
    std::array<uint32_t, 1> crc{Crc32(data)};
    std::array<uint16_t, 1> len{static_cast<uint16_t>(nBytes)};
    SendCommand(cmdData);
    // hal::WriteTo(&mEduUart_, len)
    // hal::WriteTo(&mEduUart_, data);
    // hal::WriteTo(&mEduUart_, crc);
}

auto EduUartInterface::ExecuteProgram(uint16_t programId, uint16_t queueId, uint16_t timeout)
    -> int32_t
{
    std::array<uint16_t, 3> arguments{programId, queueId, timeout};
    std::array<uint8_t, arguments.size() * 2> argumentsUint8;

    if(ArrayUint16toUint8(arguments, argumentsUint8) != 0)
    {
        PRINTF("Could not issue command, invalid arguments!\n");
        return -1;
    }

    std::array<uint8_t, argumentsUint8.size() + 1> dataBuf;
    dataBuf[0] = executeProgram;
    for(size_t i = 1; i < dataBuf.size(); i++)
    {
        dataBuf[i] = argumentsUint8[i - 1];
    }

    SendData(dataBuf);
    return 0;
}
}