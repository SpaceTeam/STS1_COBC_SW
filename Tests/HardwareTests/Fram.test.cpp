#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <Tests/HardwareTests/Utility.hpp>

#include <rodos/support/support-libs/random.h>
#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
using RODOS::PRINTF;


auto PrintDeviceId(fram::DeviceId const & deviceId) -> void;


std::int32_t errorCode = 0;


class FramTest : public RODOS::StaticThread<>
{
public:
    FramTest() : StaticThread("FramTest")
    {
    }

private:
    void init() override
    {
        errorCode = fram::Initialize();
    }

    void run() override
    {
        PRINTF("\nFRAM test\n\n");

        PRINTF("Initialize(): %i == 0\n", static_cast<int>(errorCode));
        Check(errorCode == 0);

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
        constexpr auto number1 = 0b1010'1100_b;
        constexpr auto number2 = ~number1;

        fram::WriteTo(address, Span(number1));
        PRINTF("Writing to   address 0x%08x: 0x%02x\n",
               static_cast<unsigned int>(address),
               static_cast<unsigned char>(number1));
        auto data = fram::ReadFrom<1>(address)[0];
        PRINTF("Reading from address 0x%08x: 0x%02x\n",
               static_cast<unsigned int>(address),
               static_cast<unsigned char>(data));
        Check(data == number1);

        fram::WriteTo(address, Span(number2));
        PRINTF("Writing to   address 0x%08x: 0x%02x\n",
               static_cast<unsigned int>(address),
               static_cast<unsigned char>(number2));
        fram::ReadFrom(address, Span(&data));
        PRINTF("Reading from address 0x%08x: 0x%02x\n",
               static_cast<unsigned int>(address),
               static_cast<unsigned char>(data));
        Check(data == number2);
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
