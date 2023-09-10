#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <Tests/HardwareTests/Utility.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
using RODOS::AT;
using RODOS::NOW;
using RODOS::PRINTF;
using RODOS::SECONDS;
using serial::operator""_b;


constexpr std::size_t stackSize = 5'000;

std::int32_t flashErrorCode = 0;
std::int32_t framErrorCode = 0;

auto ledGpioPin = hal::GpioPin(hal::led1Pin);


auto PrintFramDeviceId(periphery::fram::DeviceId const & deviceId) -> void;
auto BlinkLed(int frequency, int duration) -> void;


class DeviceIdsTest : public RODOS::StaticThread<stackSize>
{
public:
    DeviceIdsTest() : StaticThread("DeviceIdsTest")
    {
    }


private:
    void init() override
    {
        ledGpioPin.Direction(hal::PinDirection::out);
        ledGpioPin.Reset();
        flashErrorCode = periphery::flash::Initialize();
        framErrorCode = periphery::fram::Initialize();
    }


    void run() override
    {
        PRINTF("\nDevice IDs test\n\n");

        PRINTF("flash::Initialize(): %i == 0\n", static_cast<int>(flashErrorCode));
        Check(flashErrorCode == 0);
        PRINTF("fram::Initialize():  %i == 0\n", static_cast<int>(framErrorCode));
        Check(framErrorCode == 0);
        periphery::rf::Initialize(periphery::rf::TxType::morse);
        PRINTF("RF module initialized\n");

        PRINTF("\nFlash:\n");
        auto correctManufacturerId = 0xEF;
        auto jedecId = periphery::flash::ReadJedecId();
        PRINTF("Manufacturer ID: 0x%02x == 0x%02x\n",
               static_cast<unsigned int>(jedecId.manufacturerId),
               static_cast<unsigned int>(correctManufacturerId));
        auto flashIdsMatch = jedecId.manufacturerId == correctManufacturerId;

        auto correctFlashDeviceId = 0x4021;
        PRINTF("Device ID: 0x%04x == 0x%04x\n",
               static_cast<unsigned int>(jedecId.deviceId),
               static_cast<unsigned int>(correctFlashDeviceId));
        flashIdsMatch &= jedecId.deviceId == correctFlashDeviceId;

        PRINTF("\nFRAM:\n");
        auto correctFramDeviceId = periphery::fram::DeviceId{
            0x03_b, 0x2E_b, 0xC2_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b};
        // 0x00_b, 0x2C_b, 0xC2_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b, 0x7F_b};
        auto deviceId = periphery::fram::ReadDeviceId();
        PRINTF("Device ID: ");
        PrintFramDeviceId(deviceId);
        PRINTF(" ==\n           ");
        PrintFramDeviceId(correctFramDeviceId);
        auto framIdsMatch = deviceId == correctFramDeviceId;

        PRINTF("\nRF:\n");
        auto correctPartInfo = 0x4463;
        auto partInfo = periphery::rf::GetPartInfo();
        PRINTF("Part info: 0x%4x == 0x%4x\n", partInfo, correctPartInfo);
        auto rfIdsMatch = partInfo == correctPartInfo;

        auto blinkDuration = 3;            // s
        auto flashBlinkFrequency = 0;      // Hz
        auto framBlinkFrequency = 2;       // Hz
        auto rfBlinkFrequency = 5;         // Hz
        while(true)
        {
            if(flashIdsMatch)
            {
                BlinkLed(flashBlinkFrequency, blinkDuration);
            }
            if(framIdsMatch)
            {
                BlinkLed(framBlinkFrequency, blinkDuration);
            }
            if(rfIdsMatch)
            {
                BlinkLed(rfBlinkFrequency, blinkDuration);
            }
        }
    }
} deviceIdsTest;


auto PrintFramDeviceId(periphery::fram::DeviceId const & deviceId) -> void
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


// frequency is in Hz, duration in s
auto BlinkLed(int frequency, int duration) -> void
{
    if(frequency < 0)
    {
        return;
    }
    if(frequency == 0)
    {
        ledGpioPin.Set();
        AT(NOW() + duration * SECONDS);
    }
    else
    {
        auto ledIsOn = ledGpioPin.Read() == hal::PinState::set;
        for(int i = 0; i < 2 * frequency * duration; ++i)
        {
            ledIsOn ? ledGpioPin.Reset() : ledGpioPin.Set();
            ledIsOn = not ledIsOn;
            AT(NOW() + SECONDS / frequency / 2LL);
        }
    }
    ledGpioPin.Reset();
}
}