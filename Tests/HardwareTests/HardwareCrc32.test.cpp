#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>
#include <Sts1CobcSw/Periphery/HardwareCrc32.hpp>

#include <rodos_no_using_namespace.h>

#include <span>
#include <string_view>

namespace sts1cobcsw
{
auto test0Trailing =
    std::array<uint8_t, 8>{0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xBB, 0xA5, 0xE3};  // word-aligned
auto test1Trailing =
    std::array<uint8_t, 9>{0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xBB, 0xA5, 0xE3, 0xAB};  // 1 trailing
auto test2Trailing = std::array<uint8_t, 10>{
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xBB, 0xA5, 0xE3, 0xAB, 0xFF};  // 2 trailing
auto test3Trailing = std::array<uint8_t, 11>{
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xBB, 0xA5, 0xE3, 0xAB, 0xFF, 0x10};  // 3 trailing

class HardwareCrc32Test : public RODOS::StaticThread<>
{
    void init() override
    {
    }

    void run() override
    {
        auto testDataWord = std::to_array<uint32_t>({0xDEADBEEF, 0xCABBA5E3});
        // Online calc comparison:
        // DEADBEEFCABBA5E3: 0xA962D97B
        // DEADBEEFCABBA5E3000000AB: 0xBD5677DF
        // DEADBEEFCABBA5E30000ABFF: 0x8C49583B
        // DEADBEEFCABBA5E300ABFF10: 0xBE7357D8
        // EFBEADDEE3A5BBCA: 0x78B4282B

        periphery::EnableHardwareCrcAndDma();
        // Wait shortly for better readability in HTerm
        RODOS::AT(RODOS::NOW() + 200 * RODOS::MILLISECONDS);
        RODOS::PRINTF("DMA CRC32 0 Trailing:\n");
        periphery::DmaCrc32(std::span<uint8_t>(test0Trailing));
        RODOS::AT(RODOS::NOW() + 200 * RODOS::MILLISECONDS);
        RODOS::PRINTF("DMA CRC32 1 Trailing:\n");
        periphery::DmaCrc32(std::span<uint8_t>(test1Trailing));
        RODOS::AT(RODOS::NOW() + 200 * RODOS::MILLISECONDS);
        RODOS::PRINTF("DMA CRC32 2 Trailing:\n");
        periphery::DmaCrc32(std::span<uint8_t>(test2Trailing));
        RODOS::AT(RODOS::NOW() + 200 * RODOS::MILLISECONDS);
        RODOS::PRINTF("DMA CRC32 3 Trailing:\n");
        periphery::DmaCrc32(std::span<uint8_t>(test3Trailing));
    }
} hardwareCrc32Test;
}
