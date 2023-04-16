#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <Tests/HardwareTests/Utility.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
using RODOS::PRINTF;
using serial::operator""_b;


auto PrintDeviceId(periphery::fram::DeviceId const & deviceId) -> void;


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
        errorCode = periphery::fram::Initialize();
    }

    void run() override
    {
        PRINTF("\nFRAM test\n\n");

        PRINTF("Initialize(): %i == 0\n", static_cast<int>(errorCode));
        Check(errorCode == 0);

        PRINTF("\n");
        auto deviceId = periphery::fram::ReadDeviceId();
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
        for(uint32_t address = 0x00'00'00'00; address < 0x00'00'00'10; address++)
        {
            auto data = periphery::fram::Read<1>(address);
            PRINTF("Reading byte from address 0x%08x: 0x%02x\n",
                   static_cast<unsigned int>(address),
                   static_cast<unsigned int>(data[0]));
        }
    }
} framTest;


auto PrintDeviceId(periphery::fram::DeviceId const & deviceId) -> void
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
