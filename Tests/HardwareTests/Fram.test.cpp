#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <Tests/HardwareTests/Utility.hpp>

#include <rodos/support/support-libs/random.h>
#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <cinttypes>
#include <cstdint>


namespace sts1cobcsw
{
using RODOS::PRINTF;


auto PrintDeviceId(fram::DeviceId const & deviceId) -> void;


class FramTest : public RODOS::StaticThread<>
{
public:
    FramTest() : StaticThread("FramTest")
    {
    }

private:
    void init() override
    {
        fram::Initialize();
    }

    void run() override
    {
        PRINTF("\nFRAM test\n\n");

        PRINTF("\n");
        auto actualBaudRate = fram::ReturnActualBaudRate();
        PRINTF("Actual BaudRate: %" PRIi32 "\n", actualBaudRate);

        PRINTF("\n");
        auto deviceId = fram::ReadDeviceId();
        PRINTF("Device ID: ");
        PrintDeviceId(deviceId);
        PRINTF(" ==\n");
        PRINTF("           0x7F'7F7F'7F7F'7FC2'2E03\n");
        Check(deviceId[8] == 0x7F_b);
        Check(deviceId[7] == 0x7F_b);
        Check(deviceId[6] == 0x7F_b);
        Check(deviceId[5] == 0x7F_b);
        Check(deviceId[4] == 0x7F_b);
        Check(deviceId[3] == 0x7F_b);
        Check(deviceId[2] == 0xC2_b);
        Check(deviceId[1] == 0x2E_b);
        Check(deviceId[0] == 0x03_b);

        PRINTF("\n");
        RODOS::setRandSeed(static_cast<std::uint64_t>(RODOS::NOW()));
        constexpr uint32_t nAdressBits = 20U;
        auto address = fram::Address{RODOS::uint32Rand() % (1U << nAdressBits)};

        const size_t arraySize = 11 * 1024;  // 11 KiB
        const size_t outputAmount = 10;
        auto testArray = std::array<Byte, arraySize>{};
        std::fill(testArray.begin(), testArray.end(), Byte{0x00});

        fram::WriteTo(address, Span(testArray));
        PRINTF("Writing to   address 0x%08x:\n", static_cast<unsigned int>(address));
        for(size_t i = 0; i < outputAmount; i++)
        {
            PRINTF("0x%02x  ", static_cast<unsigned char>(testArray[i]));
        }
        auto data = fram::ReadFrom<arraySize>(address);
        PRINTF("Reading from address 0x%08x:\n", static_cast<unsigned int>(address));
        for(size_t i = 0; i < outputAmount; i++)
        {
            PRINTF("0x%02x  ", static_cast<unsigned char>(data[i]));
        }
        Check(data == testArray);

        std::fill(testArray.begin(), testArray.end(), Byte{0xFF});

        fram::WriteTo(address, Span(testArray));
        PRINTF("Writing to   address 0x%08x:\n", static_cast<unsigned int>(address));
        for(size_t i = 0; i < outputAmount; i++)
        {
            PRINTF("0x%02x  ", static_cast<unsigned char>(testArray[i]));
        }
        fram::ReadFrom(address, Span(&data));
        PRINTF("Reading from address 0x%08x:\n", static_cast<unsigned int>(address));
        for(size_t i = 0; i < outputAmount; i++)
        {
            PRINTF("0x%02x  ", static_cast<unsigned char>(data[i]));
        }
        Check(data == testArray);

        std::fill(testArray.begin(), testArray.end(), Byte{0x00});

        fram::WriteTo(address, Span(testArray));
        PRINTF("Writing to   address 0x%08x:\n", static_cast<unsigned int>(address));
        for(size_t i = 0; i < outputAmount; i++)
        {
            PRINTF("0x%02x  ", static_cast<unsigned char>(testArray[i]));
        }
        fram::ReadFrom(address, Span(&data));
        PRINTF("Reading from address 0x%08x:\n", static_cast<unsigned int>(address));
        for(size_t i = 0; i < outputAmount; i++)
        {
            PRINTF("0x%02x  ", static_cast<unsigned char>(data[i]));
        }
        Check(data == testArray);
    }
} framTest;


auto PrintDeviceId(fram::DeviceId const & deviceId) -> void
{
    PRINTF("0x");
    PRINTF("%02x", static_cast<unsigned int>(deviceId[8]));
    PRINTF("'");
    PRINTF("%02x", static_cast<unsigned int>(deviceId[7]));
    PRINTF("%02x", static_cast<unsigned int>(deviceId[6]));
    PRINTF("'");
    PRINTF("%02x", static_cast<unsigned int>(deviceId[5]));
    PRINTF("%02x", static_cast<unsigned int>(deviceId[4]));
    PRINTF("'");
    PRINTF("%02x", static_cast<unsigned int>(deviceId[3]));
    PRINTF("%02x", static_cast<unsigned int>(deviceId[2]));
    PRINTF("'");
    PRINTF("%02x", static_cast<unsigned int>(deviceId[1]));
    PRINTF("%02x", static_cast<unsigned int>(deviceId[0]));
}
}
