#include <Tests/HardwareSetup/RfLatchupProtection.hpp>

#include <Sts1CobcSw/Flash/Flash.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Rf/Rf.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <cstddef>


namespace sts1cobcsw
{
using RODOS::AT;
using RODOS::NOW;
using RODOS::PRINTF;
using RODOS::SECONDS;


namespace
{
constexpr std::size_t stackSize = 5000;

auto led1GpioPin = hal::GpioPin(hal::led1Pin);


auto PrintFramDeviceId(fram::DeviceId const & deviceId) -> void;
auto BlinkLed(int frequency, int duration) -> void;


class DeviceIdsTest : public RODOS::StaticThread<stackSize>
{
public:
    DeviceIdsTest() : StaticThread("DeviceIdsTest")
    {}


private:
    void init() override
    {
        led1GpioPin.SetDirection(hal::PinDirection::out);
        led1GpioPin.Reset();
    }


    void run() override
    {
        PRINTF("\nDevice IDs test\n\n");

        flash::Initialize();
        PRINTF("Flash initialized\n");
        fram::Initialize();
        PRINTF("FRAM initialized\n");
        DisableRfLatchupProtection();
        auto rfInitializeResult = rf::Initialize(rf::TxType::morse);
        if(rfInitializeResult.has_error())
        {
            PRINTF("Failed to initialize RF module: %s\n", ToCZString(rfInitializeResult.error()));
        }
        else
        {
            EnableRfLatchupProtection();
            PRINTF("RF module initialized\n");
        }

        PRINTF("\nFlash:\n");
        auto jedecId = flash::ReadJedecId();
        PRINTF("Manufacturer ID: 0x%02x == 0x%02x\n",
               static_cast<unsigned int>(jedecId.manufacturerId),
               static_cast<unsigned int>(flash::correctJedecId.manufacturerId));
        auto flashIdsMatch = jedecId.manufacturerId == flash::correctJedecId.manufacturerId;

        PRINTF("Device ID: 0x%04x == 0x%04x\n",
               static_cast<unsigned int>(jedecId.deviceId),
               static_cast<unsigned int>(flash::correctJedecId.deviceId));
        flashIdsMatch &= jedecId.deviceId == flash::correctJedecId.deviceId;

        PRINTF("\nFRAM:\n");
        auto deviceId = fram::ReadDeviceId();
        PRINTF("Device ID: ");
        PrintFramDeviceId(deviceId);
        PRINTF(" ==\n           ");
        PrintFramDeviceId(fram::correctDeviceId);
        auto framIdsMatch = deviceId == fram::correctDeviceId;

        auto rfIdsMatch = false;
        if(rfInitializeResult.has_value())
        {
            PRINTF("\nRF:\n");
            DisableRfLatchupProtection();
            auto partInfo = rf::ReadPartNumber();
            EnableRfLatchupProtection();
            PRINTF("Part info: 0x%4x == 0x%4x\n", partInfo, rf::correctPartNumber);
            rfIdsMatch = partInfo == rf::correctPartNumber;
        }

        auto const successBlinkDuration = 1;       // s
        auto const successBlinkFrequency = 0;      // Hz (0 means LED is on)
        auto const errorBlinkDuration = 2;         // s
        auto const flashErrorBlinkFrequency = -1;  // Hz (negative value means LED is off)
        auto const framErrorBlinkFrequency = 2;    // Hz
        auto const rfErrorBlinkFrequency = 5;      // Hz
        while(true)
        {
            if(flashIdsMatch and framIdsMatch and rfIdsMatch)
            {
                BlinkLed(successBlinkFrequency, successBlinkDuration);
            }
            else
            {
                if(not flashIdsMatch)
                {
                    BlinkLed(flashErrorBlinkFrequency, errorBlinkDuration);
                }
                if(not framIdsMatch)
                {
                    BlinkLed(framErrorBlinkFrequency, errorBlinkDuration);
                }
                if(not rfIdsMatch)
                {
                    BlinkLed(rfErrorBlinkFrequency, errorBlinkDuration);
                }
            }
            jedecId = flash::ReadJedecId();
            flashIdsMatch = jedecId.manufacturerId == flash::correctJedecId.manufacturerId
                        and jedecId.deviceId == flash::correctJedecId.deviceId;
            framIdsMatch = fram::ReadDeviceId() == fram::correctDeviceId;
            if(rfInitializeResult.has_value())
            {
                DisableRfLatchupProtection();
                rfIdsMatch = rf::ReadPartNumber() == rf::correctPartNumber;
                EnableRfLatchupProtection();
            }
        }
    }
} deviceIdsTest;


auto PrintFramDeviceId(fram::DeviceId const & deviceId) -> void
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


// Frequency is in Hz, duration in s. Negative frequencies turn LED off, 0 turns it on.
auto BlinkLed(int frequency, int duration) -> void
{
    if(frequency < 0)
    {
        led1GpioPin.Reset();
        AT(NOW() + duration * SECONDS);
        return;
    }
    if(frequency == 0)
    {
        led1GpioPin.Set();
        AT(NOW() + duration * SECONDS);
        return;
    }
    auto ledIsOn = led1GpioPin.Read() == hal::PinState::set;
    for(int i = 0; i < 2 * frequency * duration; ++i)
    {
        ledIsOn ? led1GpioPin.Reset() : led1GpioPin.Set();
        ledIsOn = not ledIsOn;
        AT(NOW() + SECONDS / frequency / 2LL);
    }
    led1GpioPin.Reset();
}
}
}
