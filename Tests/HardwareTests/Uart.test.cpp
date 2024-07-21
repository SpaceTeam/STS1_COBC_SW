//! @file
//! @brief A program to test writing to and reading from the UCI UART
//!
//! Preparation:
//!     - Connect the UCI UART to a computer to use with HTERM, Putty, etc.
//!
//! After flashing the COBC just follow the instructions on the screen.

#include <Tests/HardwareTests/RfLatchupDisablePin.hpp>

#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/Uart.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <charconv>
#include <cinttypes>
#include <cstddef>


namespace sts1cobcsw
{
auto eduUart = RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);
auto uciUart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);


template<std::size_t nDigits = 1>
auto ToChars(int i)
{
    auto string = std::array<char, nDigits>{};
    std::to_chars(string.data(), string.data() + string.size(), i);
    return string;
}


class UartTest : public RODOS::StaticThread<>
{
    void init() override
    {
#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Direction(hal::PinDirection::out);
#endif
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

#if HW_VERSION >= 27
        rfLatchupDisableGpioPin.Reset();
#endif

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
            auto writeToResult1 = hal::WriteTo(&eduUart, Span(message), 100 * MILLISECONDS);
            auto tWrite1 = NOW();

            auto answer1 = std::array<char, 1>{};

            auto tRead0 = NOW();
            auto readFromResult1 = hal::ReadFrom(&eduUart, Span(&answer1), 100 * MILLISECONDS);
            auto tRead1 = NOW();

            PRINTF("\n");
            PRINTF("tWrite1 - tWrite0 = %5" PRIi64 " us\n", (tWrite1 - tWrite0) / MICROSECONDS);
            PRINTF("tRead1  - tRead0  = %5" PRIi64 " us\n", (tRead1 - tRead0) / MICROSECONDS);
            PRINTF("writeToResult1.hasError()  = %s\n",
                   writeToResult1.has_error() ? "true" : "false");
            PRINTF("readFromResult1.hasError() = %s\n",
                   readFromResult1.has_error() ? "true" : "false");
            PRINTF("answer1 = 0x%02x\n", answer1[0]);

            PRINTF("\n");
            PRINTF("Sending the message again.");
            auto writeTimeout = 500 * MICROSECONDS;
            PRINTF(" This time with a write timeout of %" PRIi64 " us which is too short.\n",
                   writeTimeout / MICROSECONDS);

            tWrite0 = NOW();
            writeToResult1 = hal::WriteTo(&eduUart, Span(message), writeTimeout);
            tWrite1 = NOW();

            answer1[0] = 0x00;

            tRead0 = NOW();
            readFromResult1 = hal::ReadFrom(&eduUart, Span(&answer1), 100 * MILLISECONDS);
            tRead1 = NOW();

            PRINTF("\n");
            PRINTF("tWrite1 - tWrite0 = %5" PRIi64 " us\n", (tWrite1 - tWrite0) / MICROSECONDS);
            PRINTF("tRead1  - tRead0  = %5" PRIi64 " us\n", (tRead1 - tRead0) / MICROSECONDS);
            PRINTF("writeToResult1.hasError()  = %s\n",
                   writeToResult1.has_error() ? "true" : "false");
            PRINTF("readFromResult1.hasError() = %s\n",
                   readFromResult1.has_error() ? "true" : "false");
            PRINTF("answer1 = 0x%02x\n", answer1[0]);

            PRINTF("\n");
            PRINTF("Sending the message twice.");
            auto readTimeout = 1500 * MICROSECONDS;
            PRINTF(" The first time with a read timeout of %" PRIi64 " us which is too short.\n",
                   readTimeout / MICROSECONDS);

            tWrite0 = NOW();
            writeToResult1 = hal::WriteTo(&eduUart, Span(message), 100 * MILLISECONDS);
            tWrite1 = NOW();
            auto writeToResult2 = hal::WriteTo(&eduUart, Span(message), 100 * MILLISECONDS);
            auto tWrite2 = NOW();

            auto answer2 = std::array<char, 1>{};

            tRead0 = NOW();
            readFromResult1 = hal::ReadFrom(&eduUart, Span(&answer1), readTimeout);
            tRead1 = NOW();
            auto readFromResult2 = hal::ReadFrom(&eduUart, Span(&answer2), 100 * MILLISECONDS);
            auto tRead2 = NOW();

            PRINTF("\n");
            PRINTF("tWrite1 - tWrite0 = %5" PRIi64 " us\n", (tWrite1 - tWrite0) / MICROSECONDS);
            PRINTF("tWrite2 - tWrite1 = %5" PRIi64 " us\n", (tWrite2 - tWrite1) / MICROSECONDS);
            PRINTF("tRead1  - tRead0  = %5" PRIi64 " us\n", (tRead1 - tRead0) / MICROSECONDS);
            PRINTF("tRead2  - tRead1  = %5" PRIi64 " us\n", (tRead2 - tRead1) / MICROSECONDS);
            PRINTF("writeToResult1.hasError()  = %s\n",
                   writeToResult1.has_error() ? "true" : "false");
            PRINTF("writeToResult2.hasError()  = %s\n",
                   writeToResult2.has_error() ? "true" : "false");
            PRINTF("readFromResult1.hasError() = %s\n",
                   readFromResult1.has_error() ? "true" : "false");
            PRINTF("readFromResult2.hasError() = %s\n",
                   readFromResult2.has_error() ? "true" : "false");
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
