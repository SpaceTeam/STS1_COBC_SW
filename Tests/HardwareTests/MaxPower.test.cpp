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

auto led1GpioPin = hal::GpioPin(hal::led1Pin);
auto led2GpioPin = hal::GpioPin(hal::led2Pin);
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
        led1GpioPin.Direction(hal::PinDirection::out);
        led2GpioPin.Direction(hal::PinDirection::out);
    }


    void run() override
    {
        PRINTF("\nMax. power test LED thread\n\n");
        auto toggle = true;
        led1GpioPin.Set();
        TIME_LOOP(0, 100 * RODOS::MILLISECONDS)
        {
            toggle ? led2GpioPin.Set() : led2GpioPin.Reset();
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
        auto baudRate = 115'200U;
        hal::Initialize(&eduUart, baudRate);
    }


    void run() override
    {
        PRINTF("\nMax. power test UART thread\n\n");
        auto message = std::array<Byte, 100>{};
        while(true)
        {
            for(auto & i : message)
            {
                i = static_cast<Byte>(RODOS::uint32Rand() % (1U << sizeof(Byte)));
            }
            (void)hal::WriteTo(&eduUart, Span(message), uartTimeout);
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
        PRINTF("Which operation would you like to perform?\n");
        PRINTF("  1: SPI communication\n");
        PRINTF("  2: integer calculations\n");
        PRINTF("  3: floating point calculations\n");
        PRINTF("  4: ADC conversions\n");

        auto answer = std::array<Byte, 1>{};
        hal::ReadFrom(&uciUart, Span(&answer));
        PRINTF("\n");
        switch(static_cast<char>(answer[0]))
        {
            case '1':
            {
                fram::Initialize();
                PRINTF("Starting SPI communication\n");
                while(true)
                {
                    (void)fram::ReadDeviceId();
                }
                break;
            }
            case '2':
            {
                std::uint32_t number = 3;
                PRINTF("Starting integer calculation\n");
                // FIXME: This infinite loop has no side effects which causes undefined behavior
                while(true)
                {
                    number *= number + 1;
                }
                break;
            }
            case '3':
            {
                float number = 1.5;
                PRINTF("Starting floating point calculation\n");
                // FIXME: This infinite loop has no side effects which causes undefined behavior
                while(true)
                {
                    number *= number;
                }
                break;
            }
            case '4':
            {
                // TODO: Implement ADC conversions
                PRINTF("ADC conversions are not implemented yet!\n");
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
