#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <Tests/HardwareTests/Utility.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
using RODOS::PRINTF;
using serial::operator""_b;


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

        auto deviceId = periphery::fram::ReadDeviceId();
        PRINTF("Device ID: 0x");
        for(auto byte : deviceId)
        {
            PRINTF("%02x", static_cast<unsigned int>(byte));
        }
        PRINTF(" == 0x7F7F7F7F7F7FC22E03\n");
        Check(deviceId[0] == 0x7F_b);
        Check(deviceId[1] == 0x7F_b);
        Check(deviceId[2] == 0x7F_b);
        Check(deviceId[3] == 0x7F_b);
        Check(deviceId[4] == 0x7F_b);
        Check(deviceId[5] == 0x7F_b);
        Check(deviceId[6] == 0xC2_b);
        Check(deviceId[7] == 0x2E_b);
        Check(deviceId[8] == 0x03_b);
    }
} framTest;
}