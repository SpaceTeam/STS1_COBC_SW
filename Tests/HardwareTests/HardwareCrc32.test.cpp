#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>
#include <Sts1CobcSw/Periphery/HardwareCrc32.hpp>

#include <rodos_no_using_namespace.h>

#include <span>
#include <string_view>

namespace sts1cobcsw
{

auto ledPin = hal::GpioPin(hal::pa5);

class HardwareCrc32Test : public RODOS::StaticThread<>
{
    void init() override
    {
        constexpr auto uartBaudRate = 115200;
        ledPin.Direction(hal::PinDirection::out);
    }


    void run() override
    {
        periphery::EnableHardwareCrc();
        auto testDataByte = std::to_array<uint8_t>({0xD, 0xE, 0xA, 0xD, 0xB, 0xE, 0xE, 0xF});
        auto testDataDWord = std::to_array<uint32_t>({0xDEADBEEF, 0xCABBA5E3});
        auto truth = 0xA962D97B;
        // -> CRC32 should be 0xA962D97B
        auto crc32 = periphery::HardwareCrc32(std::span<uint32_t, 2>(testDataDWord));
        RODOS::PRINTF("CRC: %x\n", crc32);
    }
} hardwareCrc32Test;
}
