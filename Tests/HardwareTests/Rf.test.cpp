#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>

#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>
#include <Tests/HardwareTests/Utility.hpp>

#include <rodos_no_using_namespace.h>

#include <string_view>


namespace sts1cobcsw
{
using RODOS::PRINTF;


auto led1GpioPin = hal::GpioPin(hal::led1Pin);


class RfTest : public RODOS::StaticThread<>
{
public:
    RfTest() : StaticThread("RfTest")
    {
    }


private:
    void init() override
    {
#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Direction(hal::PinDirection::out);
#endif
    }


    void run() override
    {
#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Reset();
#endif
        led1GpioPin.Direction(hal::PinDirection::out);
        led1GpioPin.Reset();

        PRINTF("\nRF test\n\n");

        rf::Initialize(rf::TxType::packet);
        PRINTF("RF module initialized\n");

        PRINTF("\n");
        auto partInfo = rf::ReadPartInfo();
        PRINTF("Part info: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
               partInfo[0],
               partInfo[1],
               partInfo[2],
               partInfo[3],
               partInfo[4],
               partInfo[5],
               partInfo[6],
               partInfo[7]);

        PRINTF("\n");
        auto functionInfo = rf::ReadFunctionInfo();
        PRINTF("Function info: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
               functionInfo[0],
               functionInfo[1],
               functionInfo[2],
               functionInfo[3],
               functionInfo[4],
               functionInfo[5]);

        PRINTF("\n");
        auto deviceState = rf::ReadDeviceState();
        PRINTF("Device state: 0x%02x 0x%02x\n", deviceState[0], deviceState[1]);

        PRINTF("\n");
        auto partNumber = rf::ReadPartNumber();
        PRINTF("Part number: 0x%4x == 0x%4x\n", partNumber, rf::correctPartNumber);
        Check(partNumber == rf::correctPartNumber);

        PRINTF("\n");
        auto n = 1;
        PRINTF("Sending 'Hello, world!' %i times\n", n);
        auto message = std::string_view(
            "123456789,123456789,123456789,123456789,123456789,123456789,123456789,123456789,"
            "123456789,123456789");
        for(int i = 0; i < n; ++i)
        {
            led1GpioPin.Set();
            rf::Send(message.data(), message.size());
            led1GpioPin.Reset();
        }
        PRINTF("  done\n");

        // Here comes the rest of the RF test
    }
} rfTest;
}
