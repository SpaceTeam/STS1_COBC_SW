#include <Sts1CobcSw/Periphery/HardwareCrc32.hpp>

#include <stm32f4xx_crc.h>
#include <stm32f4xx_rcc.h>

#include <rodos_no_using_namespace.h>

#include <span>


namespace sts1cobcsw::periphery
{

auto EnableHardwareCrc() -> void{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);
}

auto HardwareCrc32(std::span<uint32_t> data) -> uint32_t
{
    RODOS::PRINTF("size: %u\n", data.size());
    for(auto && e : data)
    {
        RODOS::PRINTF("%u\n", e);
    }

    return CRC_CalcBlockCRC(data.data(), data.size());
}
}  // namespace sts1cobcsw::periphery
