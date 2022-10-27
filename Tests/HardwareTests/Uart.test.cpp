//! @file
//! @brief A program to test writing to and reading from the UCI UART
//!
//! Preparation:
//!     - Connect the UCI UART to a computer to use with HTERM, Putty, etc.
//!
//! After flashing the COBC just follow the instructions on the screen.

#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <charconv>
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
        constexpr auto uartBaudRate = 115200;
        eduUart.init(uartBaudRate);
        uciUart.init(uartBaudRate);
    }


    void run() override
    {
        hal::WriteTo(&uciUart, "Hello UCI UART!\n");

        while(true)
        {
            constexpr auto bufferSize = 5;
            auto buffer = std::array<std::byte, bufferSize>{};

            hal::WriteTo(&uciUart, "\nPlease send ");
            auto bufferSizeString = ToChars(bufferSize);
            hal::WriteTo(&uciUart, std::span(bufferSizeString));
            hal::WriteTo(&uciUart, " characters\n");
            hal::ReadFrom(&uciUart, std::span(buffer));
            hal::WriteTo(&uciUart, "You sent: ");
            hal::WriteTo(&uciUart, std::span(buffer));
            hal::WriteTo(&uciUart, "\n");
        }
    }
};


auto const uartTest = UartTest();
}
