#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>
#include <Sts1CobcSw/Periphery/HardwareCrc32.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <span>

namespace sts1cobcsw
{
auto test0Trailing =
    std::to_array<Byte>{0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xBB, 0xA5, 0xE3};  // word-aligned
auto test1Trailing =
    std::to_array<Byte>{0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xBB, 0xA5, 0xE3, 0xAB};  // 1 trailing
auto test2Trailing = std::to_array<Byte>{
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xBB, 0xA5, 0xE3, 0xAB, 0xFF};  // 2 trailing
auto test3Trailing = std::to_array<Byte>{
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xBB, 0xA5, 0xE3, 0xAB, 0xFF, 0x10};  // 3 trailing
auto testWords = std::to_array<std::uint32_t>{0xDEADBEEF, 0xCABBA5E3};

class HardwareCrc32Test : public RODOS::StaticThread<>
{
public:
    HardwareCrc32Test() : StaticThread("HardwareCrc32Test")
    {
    }


private:
    void init() override
    {
    }

    void run() override
    {
        // Online CRC32 (MPEG-2) comparison:
        // DEADBEEFCABBA5E3:            0xA962D97B
        // DEADBEEFCABBA5E3000000AB:    0xBD5677DF
        // DEADBEEFCABBA5E30000ABFF:    0x8C49583B
        // DEADBEEFCABBA5E300ABFF10:    0xBE7357D8
        // EFBEADDEE3A5BBCA:            0x78B4282B

        utility::InitializeCrc32Hardware();
        // Wait shortly for better readability in HTerm

        // Currently the DMA CRC32 is implemented in a blocking manner, so we can just test like this
        // RODOS::AT(RODOS::NOW() + 200 * RODOS::MILLISECONDS);
        RODOS::PRINTF("DMA CRC32 0 Trailing:\n");
        RODOS::PRINTF("CRC32: %u\n", utility::ComputeCrc32(utility::Span(test0Trailing)));

        // RODOS::AT(RODOS::NOW() + 200 * RODOS::MILLISECONDS);
        RODOS::PRINTF("DMA CRC32 1 Trailing:\n");
        RODOS::PRINTF("CRC32: %u\n", utility::ComputeCrc32(utility::Span(test1Trailing)));

        // RODOS::AT(RODOS::NOW() + 200 * RODOS::MILLISECONDS);
        RODOS::PRINTF("DMA CRC32 2 Trailing:\n");
        RODOS::PRINTF("CRC32: %u\n", utility::ComputeCrc32(utility::Span(test2Trailing)));

        // RODOS::AT(RODOS::NOW() + 200 * RODOS::MILLISECONDS);
        RODOS::PRINTF("DMA CRC32 3 Trailing:\n");
        RODOS::PRINTF("CRC32: %u\n", utility::ComputeCrc32(utility::Span(test3Trailing)));
    }
} hardwareCrc32Test;
}
