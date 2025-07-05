#include <Sts1CobcSw/Bootloader/Spi.hpp>

#include <Sts1CobcSw/Bootloader/BusyWait.hpp>
#include <Sts1CobcSw/Bootloader/stm32f411xe.h>
#include <Sts1CobcSw/Bootloader/UciUart.hpp>  //test only

namespace sts1cobcsw::bootloader::utilities
{
// NOLINTBEGIN(*no-int-to-ptr, *cstyle-cast)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

auto PrintHexString(char const * string, int stringLenght) -> void
{
    char idString[3*stringLenght];
    int len = 0;
    for(unsigned int i = 0; i < stringLenght; ++i)
    {
        auto byte = static_cast<unsigned char>(string[i]);
        // High nibble
        idString[len++] = "0123456789ABCDEF"[byte >> 4];
        // Low nibble
        idString[len++] = "0123456789ABCDEF"[byte & 0x0F];
        if(len < 3*stringLenght-1)
        {
            idString[len++] = ' ';
        }
    }
    idString[len] = '\0';

    uciuart::Write(idString);
}

#pragma GCC diagnostic pop
// NOLINTEND(*no-int-to-ptr, *cstyle-cast)
}
