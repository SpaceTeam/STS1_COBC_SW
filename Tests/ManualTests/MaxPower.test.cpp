#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Uart.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos/support/support-libs/random.h>
#include <rodos_no_using_namespace.h>

#include <array>
#include <cinttypes>
#include <cmath>
#include <cstdint>


namespace sts1cobcsw
{
using RODOS::PRINTF;


constexpr auto uartTimeout = 100 * ms;

auto led1GpioPin = hal::GpioPin(hal::led1Pin);
auto led2GpioPin = hal::GpioPin(hal::led2Pin);
auto eduUart = RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);
auto uciUart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);


auto IsPrime(std::int32_t number) -> bool;


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
        TIME_LOOP(0, value_of(100 * ms))
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
        while(true)
        {
            PRINTF("\nMax. power test main thread\n\n");
            PRINTF("Which operation would you like to perform?\n");
            PRINTF("  1: SPI communication\n");
            PRINTF("  2: Searching for large prime numbers\n");
            PRINTF("  3: Computing pi\n");
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
                    PRINTF("Searching for large prime numbers ...");
                    auto const max = INT32_MAX;
                    std::int32_t largestPrime = 0;
                    for(std::int32_t i = 0; i <= max; ++i)
                    {
                        if(IsPrime(i))
                        {
                            largestPrime = i;
                        }
                    }
                    PRINTF(" done\n");
                    PRINTF("The largest prime number smaller than %" PRIi32 " is %" PRIi32 ".\n\n",
                           max,
                           largestPrime);
                    break;
                }
                case '3':
                {
                    PRINTF("Starting to compute pi ...");
                    float k = 0.0F;
                    float pi = 0.0F;
                    for(std::int32_t i = 0; i < INT32_MAX; ++i)
                    {
                        pi += std::pow(-1.0F, k) / (2.0F * k + 1.0F);
                    }
                    pi *= 4.0F;
                    PRINTF(" done\n");
                    PRINTF("Pi is approximately %f\n\n", static_cast<double>(pi));
                    break;
                }
                case '4':
                {
                    // TODO: Implement ADC conversions
                    PRINTF("ADC conversions are not implemented yet!\n\n");
                    break;
                }
                default:
                {
                    PRINTF("Unknown command\n\n");
                    break;
                }
            }
        }
    }
} maxPowerTestMainThread;


auto IsPrime(std::int32_t number) -> bool
{
    if(number <= 1)
    {
        return false;
    }
    for(std::int32_t i = 2; i * i <= number; ++i)
    {
        if(number % i == 0)
        {
            return false;
        }
    }
    return true;
}
}
