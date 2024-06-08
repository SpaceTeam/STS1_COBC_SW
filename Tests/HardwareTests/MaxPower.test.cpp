#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Uart.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <rodos/support/support-libs/random.h>
#include <rodos_no_using_namespace.h>

#include <array>
#include <cstdint>


namespace sts1cobcsw
{
using RODOS::PRINTF;


constexpr auto uartTimeout = 100 * RODOS::MILLISECONDS;

auto pinsToTest = std::to_array<hal::GpioPin>({hal::led1Pin, hal::led2Pin});
auto eduUart = RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);
auto uciUart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);


class MaxPowerTestLedThread : public RODOS::StaticThread<>
{
public:
    MaxPowerTestLedThread() : StaticThread("MaxPowerTestLedThread")
    {
    }


private:
    void init() override
    {
        for(auto & pin : pinsToTest)
        {
            pin.Direction(hal::PinDirection::out);
        }
    }


    void run() override
    {
        PRINTF("\nMax. power test LED thread\n\n");
        auto toggle = true;

        TIME_LOOP(0, 1000 * RODOS::MILLISECONDS)
        {
            for(auto & pin : pinsToTest)
            {
                toggle ? pin.Set() : pin.Reset();
            }
            toggle = not toggle;
        }
    }
} maxPowerTestLedThread;


class MaxPowerTestUartThread : public RODOS::StaticThread<>
{
public:
    MaxPowerTestUartThread() : StaticThread("MaxPowerTestUartThread")
    {
    }


private:
    void init() override
    {
        RODOS::setRandSeed(static_cast<std::uint64_t>(RODOS::NOW()));
        auto uartBaudRate = 115'200U;
        hal::Initialize(&eduUart, uartBaudRate);
    }


    void run() override
    {
        PRINTF("\nMax. power test UART thread\n\n");

        auto message = std::to_array<Byte>({0x00_b,
                                            0x00_b,
                                            0x00_b,
                                            0x00_b,
                                            0x00_b,
                                            0x00_b,
                                            0x00_b,
                                            0x00_b,
                                            0x00_b,
                                            0x00_b,
                                            0x00_b,
                                            0x00_b});

        while(true)
        {
            // generate random message content
            for(auto & i : message)
            {
                i = static_cast<Byte>(RODOS::uint32Rand() % (1U << sizeof(Byte)));
            }

            PRINTF("Send random message to edu over UART\n");
            (void)hal::WriteTo(&eduUart, Span(message), uartTimeout);  // use non blocking call
        }
    }
} maxPowerTestUartThread;


class MaxPowerTestMainThread : public RODOS::StaticThread<>
{
public:
    MaxPowerTestMainThread() : StaticThread("MaxPowerTestMainThread")
    {
    }


private:
    void init() override
    {
        auto const baudRate = 115'200;
        hal::Initialize(&uciUart, baudRate);
    }


    void run() override
    {
        PRINTF("\nMax. power test main thread\n\n");
        PRINTF("Select operation to perform:\n");
        PRINTF("[1 - SPI Com]\n");
        PRINTF("[2 - integer  calculation]\n");
        PRINTF("[3 - floating calculation]\n");
        PRINTF("[4 - ADC]\n");

        auto command = std::array<Byte, 1>{};
        hal::ReadFrom(&uciUart, Span(&command));
        PRINTF("\n");
        switch(static_cast<char>(command[0]))
        {
            case '1':
            {
                // SPI communication
                fram::Initialize();
                PRINTF("Start SPI communication\n");
                while(true)
                {
                    (void)fram::ReadDeviceId();
                }
                break;
            }
            case '2':
            {
                // integer calculation to simulate load
                uint32_t number = 3;
                PRINTF("Start integer calculation\n");
                while(true)
                {
                    number *= number;
                }
                break;
            }
            case '3':
            {
                // floating point calculation to simulate load
                float number = 1.5;
                PRINTF("Start floating point calculation\n");
                while(true)
                {
                    number *= number;
                }
                break;
            }
            case '4':
            {
                // ToDo add ADC
                PRINTF("ADC not supported!\n");
                break;
            }
            default:
            {
                PRINTF("Unknown command\n");
                break;
            }
        }
    }
} maxPowerTestMainThread;
}
