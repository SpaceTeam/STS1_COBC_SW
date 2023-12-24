//! @file
//! @brief A program to test writing to and reading from the UCI UART
//!
//! Preparation:
//!     - Connect the UCI UART to a computer to use with HTERM, Putty, etc.
//!
//! After flashing the COBC just follow the instructions on the screen.

#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>
#include <Sts1CobcSw/Hal/Uart.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <charconv>
#include <cinttypes>
#include <cstddef>
#include <span>


namespace sts1cobcsw
{
auto eduUart = RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);
auto uciUart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);


template<std::size_t nDigits = 1>
auto ToChars(int i)
{
    auto string = std::array<char, nDigits>{};
    std::to_chars(data(string), data(string) + size(string), i);
    return string;
}


// TODO: Make this test usable even if there is no EDU connected/working. This means two threads,
// one for EDU one for UCI and it should use suspendUntilDataReady() and maybe a timeout, etc.
class UartTest : public RODOS::StaticThread<>
{
    void init() override
    {
        auto uartBaudRate = 115'200U;
        hal::Initialize(&eduUart, uartBaudRate);
        hal::Initialize(&uciUart, uartBaudRate);
    }


    void run() override
    {
        using RODOS::MICROSECONDS;
        using RODOS::MILLISECONDS;
        using RODOS::NOW;
        using RODOS::PRINTF;

        PRINTF("\n");
        PRINTF("Is an EDU connected to test the non-blocking UART functions? (y/n)\n");
        auto isEduConnected = std::array<Byte, 1>{};
        hal::ReadFrom(&uciUart, Span(&isEduConnected));
        if(isEduConnected[0] == static_cast<Byte>('y'))
        {
            // Update time command with timestamp = 0x00'00'00'00
            auto message = std::to_array<Byte>({0x8B_b,
                                                0x05_b,
                                                0x00_b,
                                                0x06_b,
                                                0x00_b,
                                                0x00_b,
                                                0x00_b,
                                                0x00_b,
                                                0x05_b,
                                                0xE7_b,
                                                0xFE_b,
                                                0xF5_b});
            PRINTF("\n");
            PRINTF("Sending a message containing an update time command to the EDU.\n");
            PRINTF("message.size() = %i\n", static_cast<int>(message.size()));

            auto tWrite0 = NOW();
            hal::WriteTo(&eduUart, Span(message), 1000 * MILLISECONDS);
            auto tWrite1 = NOW();

            auto answer1 = std::array<char, 1>{};

            auto tRead0 = NOW();
            hal::ReadFrom(&eduUart, Span(&answer1), 1000 * MILLISECONDS);
            auto tRead1 = NOW();

            PRINTF("\n");
            PRINTF("tWrite1 - tWrite0 = %8" PRIi64 " us\n", (tWrite1 - tWrite0) / MICROSECONDS);
            PRINTF("tRead1  - tRead0  = %8" PRIi64 " us\n", (tRead1 - tRead0) / MICROSECONDS);
            PRINTF("answer1 = 0x%02x\n", answer1[0]);

            PRINTF("\n");
            PRINTF("Sending the message again.");
            auto timeout = 500 * MICROSECONDS;
            PRINTF(" This time with a timeout of %" PRIi64 " us which is too short.\n",
                   timeout / MICROSECONDS);

            tWrite0 = NOW();
            hal::WriteTo(&eduUart, Span(message), timeout);
            tWrite1 = NOW();

            answer1[0] = 0x00;

            tRead0 = NOW();
            hal::ReadFrom(&eduUart, Span(&answer1), 1000 * MILLISECONDS);
            tRead1 = NOW();

            PRINTF("\n");
            PRINTF("tWrite1 - tWrite0 = %8" PRIi64 " us\n", (tWrite1 - tWrite0) / MICROSECONDS);
            PRINTF("tRead1  - tRead0  = %8" PRIi64 " us\n", (tRead1 - tRead0) / MICROSECONDS);
            PRINTF("answer1 = 0x%02x\n", answer1[0]);

            PRINTF("\n");
            PRINTF("Sending the message twice.\n");

            tWrite0 = NOW();
            hal::WriteTo(&eduUart, Span(message), 1000 * MILLISECONDS);
            tWrite1 = NOW();
            hal::WriteTo(&eduUart, Span(message), 1000 * MILLISECONDS);
            auto tWrite2 = NOW();

            auto answer2 = std::array<char, 1>{};

            tRead0 = NOW();
            hal::ReadFrom(&eduUart, Span(&answer1), 1000 * MILLISECONDS);
            tRead1 = NOW();
            hal::ReadFrom(&eduUart, Span(&answer2), 1000 * MILLISECONDS);
            auto tRead2 = NOW();

            PRINTF("\n");
            PRINTF("tWrite1 - tWrite0 = %8" PRIi64 " us\n", (tWrite1 - tWrite0) / MICROSECONDS);
            PRINTF("tWrite2 - tWrite1 = %8" PRIi64 " us\n", (tWrite2 - tWrite1) / MICROSECONDS);
            PRINTF("tRead1  - tRead0  = %8" PRIi64 " us\n", (tRead1 - tRead0) / MICROSECONDS);
            PRINTF("tRead2  - tRead1  = %8" PRIi64 " us\n", (tRead2 - tRead1) / MICROSECONDS);
            PRINTF("answer1 = 0x%02x\n", answer1[0]);
            PRINTF("answer2 = 0x%02x\n", answer2[0]);

            PRINTF("\n");
            PRINTF("Done with non-blocking EDU UART functions.\n");
        }

        PRINTF("\n");

        while(true)
        {
            constexpr auto bufferSize = 5;
            auto buffer = std::array<std::byte, bufferSize>{};

            hal::WriteTo(&uciUart, Span('\r'));
            hal::WriteTo(&uciUart, Span('\n'));
            hal::WriteTo(&uciUart, Span("Please send "));
            hal::WriteTo(&uciUart, Span(ToChars(bufferSize)));
            hal::WriteTo(&uciUart, Span(" characters via UCI UART"));
            hal::WriteTo(&uciUart, Span('\r'));
            hal::WriteTo(&uciUart, Span('\n'));
            hal::ReadFrom(&uciUart, Span(&buffer));
            hal::WriteTo(&uciUart, Span("You sent: "));
            hal::WriteTo(&uciUart, Span(buffer));
            hal::WriteTo(&uciUart, Span('\r'));
            hal::WriteTo(&uciUart, Span('\n'));
        }
    }
} uartTest;
}
