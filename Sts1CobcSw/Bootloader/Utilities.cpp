#include <Sts1CobcSw/Bootloader/Utilities.hpp>

#include <Sts1CobcSw/Bootloader/stm32f411xe.h>
#include <Sts1CobcSw/Bootloader/UciUart.hpp>  //test only

namespace sts1cobcsw::utilities
{
// NOLINTBEGIN(*no-int-to-ptr, *cstyle-cast)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

auto PrintHexString(char const * string, int stringLenght) -> void
{
    char idString[3 * stringLenght];
    int len = 0;
    for(unsigned int i = 0; i < stringLenght; ++i)
    {
        auto byte = static_cast<unsigned char>(string[i]);
        // High nibble
        idString[len++] = "0123456789ABCDEF"[byte >> 4];
        // Low nibble
        idString[len++] = "0123456789ABCDEF"[byte & 0x0F];
        if(len < 3 * stringLenght - 1)
        {
            idString[len++] = ' ';
        }
    }
    idString[len] = '\0';

    uciuart::Write(idString);
}

auto PrintBinString(char const * string, int stringLength) -> void
{
    char binString[9 * stringLength];  // 8 bits + space per byte
    int len = 0;
    for(unsigned int i = 0; i < stringLength; ++i)
    {
        unsigned char byte = static_cast<unsigned char>(string[i]);
        for(int bit = 7; bit >= 0; --bit)
        {
            binString[len++] = (byte & (1 << bit)) ? '1' : '0';
        }
        if(len < 9 * stringLength - 1)
        {
            binString[len++] = ' ';
        }
    }
    binString[len] = '\0';

    uciuart::Write(binString);
}

auto PrintDecString(char const * string, int stringLength) -> void
{
    char decString[5 * stringLength];  // Up to 3 digits + space + null terminator per byte
    int len = 0;
    for(unsigned int i = 0; i < stringLength; ++i)
    {
        auto byte = static_cast<unsigned char>(string[i]);
        // Convert byte to decimal string
        if(byte >= 100)
        {
            decString[len++] = '0' + (byte / 100);
            decString[len++] = '0' + ((byte / 10) % 10);
            decString[len++] = '0' + (byte % 10);
        }
        else if(byte >= 10)
        {
            decString[len++] = '0' + (byte / 10);
            decString[len++] = '0' + (byte % 10);
        }
        else
        {
            decString[len++] = '0' + byte;
        }
        if(len < 5 * stringLength - 1)
        {
            decString[len++] = ' ';
        }
    }
    decString[len] = '\0';

    uciuart::Write(decString);
}

#pragma GCC diagnostic pop
// NOLINTEND(*no-int-to-ptr, *cstyle-cast)
}
