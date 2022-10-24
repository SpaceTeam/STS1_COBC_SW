//! @file
//! @brief A program to test reading from the UCI/EDU UART
//!
//! Preparation:
//!     - Prepare any program that prints/reads to/from the EDU UART on the EDU
//!     - Connect the UCI UART to a computer to use with HTERM, Putty, etc.
//!
//! After flashing the COBC, the program will
//! 1) Reflect messages from the EDU and print them to the UCI UART
//! 2) Reflect messages from the UCI

#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <cstddef>
#include <span>


namespace sts1cobcsw
{
auto eduUart = RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);
auto uciUart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);


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
        while(true)
        {
            constexpr auto bufferSize = 64;

            // Check EDU UART
            auto eduBuffer = std::array<std::byte, bufferSize>{};
            hal::ReadFrom(&eduUart, std::span(eduBuffer));
            hal::WriteTo(&eduUart, std::span(eduBuffer));
            hal::WriteTo(&uciUart, std::span(eduBuffer));

            // Check UCI UART
            auto uciBuffer = std::array<std::byte, bufferSize>{};
            hal::ReadFrom(&uciUart, std::span(uciBuffer));
            hal::WriteTo(&uciUart, std::span(uciBuffer));
        }
    }
};


auto const uartTest = UartTest();
}
