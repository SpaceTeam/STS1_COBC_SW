#include <Sts1CobcSw/ChannelCoding/ChannelCoding.hpp>
#include <Sts1CobcSw/ChannelCoding/ReedSolomon.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Rf/Rf.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <cstdint>
#include <span>
#include <utility>


namespace sts1cobcsw
{
using RODOS::PRINTF;


namespace
{
auto led1GpioPin = hal::GpioPin(hal::led1Pin);


auto Receive(std::uint32_t baudRate) -> void;
auto Print(std::span<Byte const> data) -> void;


class RfReceiveTest : public RODOS::StaticThread<5000>
{
public:
    RfReceiveTest() : StaticThread("RfReceiveTest")
    {}


private:
    auto init() -> void override
    {
        led1GpioPin.SetDirection(hal::PinDirection::out);
        led1GpioPin.Reset();
    }


    auto run() -> void override
    {
        PRINTF("\nRF receive test\n\n");
        // We need to initialize the FRAM too because the RF code uses persistent variables
        fram::Initialize();
        auto initializeResult = rf::Initialize();
        if(initializeResult.has_error())
        {
            PRINTF("Failed to initialize RF module: %s\n", ToCZString(initializeResult.error()));
            return;
        }
        rf::DisableTx();
        PRINTF("RF module initialized, TX disabled\n");
        Receive(9600);
        PRINTF("-> done\n");
    }
} rfReceiveTest;


auto Receive(std::uint32_t baudRate) -> void
{
    PRINTF("\n");
    PRINTF("Receiving with %i baud\n", static_cast<int>(baudRate));
    rf::SetRxDataRate(baudRate);

    auto receivedData = std::array<Byte, rs::blockLength>{};
    static constexpr auto rxTimeout = 5 * s;
    PRINTF("Waiting %i s to receive %i bytes\n",
           static_cast<int>(rxTimeout / s),
           static_cast<int>(receivedData.size()));
    auto nReceivedBytes = rf::Receive(Span(&receivedData), rxTimeout);
    PRINTF("Received %i bytes:\n", static_cast<int>(nReceivedBytes));
    auto decodeResult = tc::Decode(receivedData);
    if(decodeResult.has_error())
    {
        PRINTF("Undecodeable!\n");
    }
    else
    {
        PRINTF("Correctable Errors: %u\n", decodeResult.value());
        Print(Span(receivedData).first(nReceivedBytes));
    }
}


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
}
