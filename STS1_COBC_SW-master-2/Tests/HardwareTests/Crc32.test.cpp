//#include <Sts1CobcSw/Util/Util.hpp>

//#include <stm32f4xx_crc.h>
//#include <stm32f4xx_rcc.h>

// ifdef CRC
// store macro value
// undefine
// after include, restore


#include <rodos_no_using_namespace.h>

#include <array>
#include <cstdint>


namespace sts1cobcsw
{
std::array<std::uint8_t, 4> swData = {0xAA, 0x00, 0xFF, 0x00};
std::uint32_t hwData = 0xAA00FF00;

class Crc32Test : public RODOS::StaticThread<>
{
    void init() override
    {
        // RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);
    }

    void run() override
    {
        // auto crcHw = CRC_CalcCRC(hwData);
        // auto crcSw = util::Crc32(swData);

        // RODOS::PRINTF("Hardware CRC: %c", (crcHw >> 24) & 0xFF);
        // RODOS::PRINTF("%c", (crcHw >> 16) & 0xFF);
        // RODOS::PRINTF("%c", (crcHw >> 8) & 0xFF);
        // RODOS::PRINTF("%c\n\n", crcHw & 0xFF);
        //
        // RODOS::PRINTF("Software CRC: %c", (crcSw >> 24) & 0xFF);
        // RODOS::PRINTF("%c", (crcSw >> 16) & 0xFF);
        // RODOS::PRINTF("%c", (crcSw >> 8) & 0xFF);
        // RODOS::PRINTF("%c\n\n", crcSw & 0xFF);
    }
};


auto const crc32Test = Crc32Test();
}
