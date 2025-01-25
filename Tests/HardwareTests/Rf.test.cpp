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
        auto partNumber = rf::ReadPartNumber();
        PRINTF("Part number: 0x%4x == 0x%4x\n", partNumber, rf::correctPartNumber);
        Check(partNumber == rf::correctPartNumber);

        // PRINTF("\n");
        // auto n = 1;
        // auto message = std::string_view(
        //     "123456789,123456789,123456789,123456789,123456789,123456789,123456789,123456789,"
        //     "123456789,123456789");
        // PRINTF("Sending a %i bytes long test message %i time(s)\n", message.size(), n);
        // for(int i = 0; i < n; ++i)
        // {
        //     led1GpioPin.Set();
        //     rf::Send(message.data(), message.size());
        //     led1GpioPin.Reset();
        // }
        // PRINTF("  done\n");

        PRINTF("\n");
        PRINTF("Waiting to receive test data\n");
        auto receivedData = rf::ReceiveTestData();
        PRINTF("Received data: ");
        for(auto byte : receivedData)
        {
            PRINTF("0x%02x ", static_cast<unsigned int>(byte));
        }
        PRINTF("  done\n");
    }
} rfTest;
}
