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
            constexpr auto readBufferSize = 64;

            // Check EDU UART
            auto eduReadBuffer = std::array<std::byte, readBufferSize>{};
            auto nReadBytes = hal::ReadFrom(&eduUart, std::span(eduReadBuffer));
            if(nReadBytes > 0)
            {
                auto trimmedMessage = std::span(begin(eduReadBuffer), nReadBytes);
                // Reflect to EDU and also print to UCI UART
                hal::WriteTo(&eduUart, trimmedMessage);
                hal::WriteTo(&uciUart, trimmedMessage);
            }

            // Check UCI UART
            auto uciReadBuffer = std::array<std::byte, readBufferSize>{};
            nReadBytes = hal::ReadFrom(&uciUart, std::span(uciReadBuffer));
            if(nReadBytes > 0)
            {
                auto trimmedUciMessage = std::span(begin(uciReadBuffer), nReadBytes);
                // Reflect to UCI UART
                hal::WriteTo(&uciUart, trimmedUciMessage);
            }
        }
    }
};


auto const uartTest = UartTest();
}
