#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>

#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>
#include <Tests/HardwareTests/Utility.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using RODOS::PRINTF;


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

        PRINTF("\nRF test\n\n");

        rf::Initialize(rf::TxType::packet);
        PRINTF("RF module initialized\n");

        PRINTF("\n");
        auto deviceState = rf::ReadDeviceState();
        PRINTF("Device state: 0x%02x 0x%02x\n", deviceState[0], deviceState[1]);

        PRINTF("\n");
        auto partNumber = rf::ReadPartNumber();
        PRINTF("Part number: 0x%4x == 0x%4x\n", partNumber, rf::correctPartNumber);
        Check(partNumber == rf::correctPartNumber);

        PRINTF("\n");
        PRINTF("Sending 'Hello, world!'...\n");
        auto message = std::array{"Hello, world!"};
        rf::Send(message.data(), message.size());
        PRINTF("  done\n");

        // Here comes the rest of the RF test
    }
} rfTest;
}
