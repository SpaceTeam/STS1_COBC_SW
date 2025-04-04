#include <Tests/HardwareSetup/RfLatchupProtection.hpp>

#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Rf/Rf.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <rodos_no_using_namespace.h>

#include <array>


namespace sts1cobcsw
{
using RODOS::PRINTF;

auto led1GpioPin = hal::GpioPin(hal::led1Pin);


auto Print(std::span<Byte const> data) -> void;


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
        auto receiveTestDataResult = rf::ReceiveTestData();
        if(receiveTestDataResult.has_error())
        {
            PRINTF("Error: %i\n", static_cast<int>(receiveTestDataResult.error()));
        }
        else
        {
            auto & receivedTestData = receiveTestDataResult.value();
            PRINTF("Received data:");
            Print(Span(receivedTestData));
            PRINTF("-> done\n");
        }

        // A fully encoded transfer frame is 255 * 1.5 = 382.5 -> 383 bytes long
        auto receivedData = std::array<Byte, 383>{};
        static constexpr auto rxTimeout = 5 * s;
        PRINTF("\n");
        PRINTF("Waiting %i s to receive %i bytes\n",
               static_cast<int>(rxTimeout / s),
               static_cast<int>(receivedData.size()));
        auto receiveResult = rf::Receive(Span(&receivedData), rxTimeout);
        if(receiveResult.has_error())
        {
            PRINTF("Error: %i\n", static_cast<int>(receiveResult.error()));
            return;
        }
        PRINTF("Received data:");
        Print(Span(receivedData));
        PRINTF("-> done\n");
    }
} rfReceiveTest;


auto Print(std::span<Byte const> data) -> void
{
    static constexpr auto valuesPerLine = 100 / 5;
    for(auto i = 0U; i < data.size(); ++i)
    {
        PRINTF("0x%02x ", static_cast<unsigned int>(data[i]));
        if(i % valuesPerLine == valuesPerLine - 1)
        {
            PRINTF("\n");
        }
    }
    if(data.size() % valuesPerLine != 0)
    {
        PRINTF("\n");
    }
}
}
