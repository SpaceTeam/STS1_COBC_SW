#include <Tests/HardwareSetup/RfLatchupProtection.hpp>

#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Rf/Rf.hpp>

#include <rodos_no_using_namespace.h>

#include <array>


namespace sts1cobcsw
{
using RODOS::PRINTF;

auto led1GpioPin = hal::GpioPin(hal::led1Pin);


class RfReceiveTest : public RODOS::StaticThread<5'000>
{
public:
    RfReceiveTest() : StaticThread("RfReceiveTest")
    {
    }


private:
    void init() override
    {
        led1GpioPin.SetDirection(hal::PinDirection::out);
        led1GpioPin.Reset();
    }


    void run() override
    {
        PRINTF("\nRF receive test\n\n");

        rf::Initialize(rf::TxType::packet);
        PRINTF("RF module initialized\n");

        PRINTF("\n");
        PRINTF("Waiting to receive test data\n");
        DisableRfLatchupProtection();
        auto receiveTestDataResult = rf::ReceiveTestData();
        EnableRfLatchupProtection();
        if(receiveTestDataResult.has_error())
        {
            PRINTF("Error: %i\n", static_cast<int>(receiveTestDataResult.error()));
            return;
        }
        auto receivedData = receiveTestDataResult.value();
        PRINTF("Received data:");
        static constexpr auto valuesPerLine = 100 / 5;
        for(auto i = 0U; i < receivedData.size(); ++i)
        {
            if(i % valuesPerLine == 0)
            {
                PRINTF("\n");
            }
            PRINTF("0x%02x ", static_cast<unsigned int>(receivedData[i]));
        }
        PRINTF("\n");
        PRINTF("-> done\n");
    }
} rfReceiveTest;
}
