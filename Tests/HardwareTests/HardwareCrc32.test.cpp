#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>
#include <Sts1CobcSw/Periphery/HardwareCrc32.hpp>

#include <rodos_no_using_namespace.h>

#include <span>
#include <string_view>

namespace sts1cobcsw
{

class HardwareCrc32Test : public RODOS::StaticThread<>
{
    void init() override
    {
    }

    void run() override
    {
        auto testDataByte =
            std::to_array<uint8_t>({0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xBB, 0xA5, 0xE3});
        auto testDataWord = std::to_array<uint32_t>({0xDEADBEEF, 0xCABBA5E3});
        // For online calc comparison:
        // DEADBEEFCABBA5E3: 0xA962D97B
        // EFBEADDEE3A5BBCA: 0x78B4282B
        // // -> CRC32 should be 0xA962D97B

        RODOS::PRINTF("DMA CRC32:\n");
        // Wait shortly for better readability in HTerm
        RODOS::AT(RODOS::NOW() + 200 * RODOS::MILLISECONDS);
        periphery::EnableHardwareCrcAndDma();
        periphery::DmaCrc32(std::span<uint32_t>(testDataWord));
    }
} hardwareCrc32Test;
}
