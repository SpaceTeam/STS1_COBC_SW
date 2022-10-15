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
//! ```

#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/Gpio.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>

#include <rodos_no_using_namespace.h>

#include <array>


namespace RODOS
{
// NOLINTNEXTLINE(readability-identifier-naming)
extern HAL_UART uart_stdout;
}


namespace sts1cobcsw
{
auto greenLed = RODOS::HAL_GPIO(hal::ledPin);
auto eduUart = RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);
auto uciUart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);
constexpr auto receiveBufferSize = 64;


class UartReadTest : public RODOS::StaticThread<>
{
    void init() override
    {
        hal::SetPinDirection(&greenLed, hal::PinDirection::out);
        eduUart.init();
        uciUart.init();
    }


    void run() override
    {
        while(true)
        {
            // Check EDU UART
            // Use an array so we can use WriteTo immediately
            std::array<uint8_t, receiveBufferSize> eduReceiveBuffer = {};
            auto nEduReceivedBytes =
                eduUart.read(eduReceiveBuffer.begin(), eduReceiveBuffer.size());

            if(nEduReceivedBytes > 0)
            {
                auto trimmedEduMessage = std::span{eduReceiveBuffer.begin(), nEduReceivedBytes};
                // Reflect to EDU and also print to UCI UART
                hal::WriteTo(&eduUart, trimmedEduMessage);
                hal::WriteTo(&uciUart, trimmedEduMessage);
            }

            // Check UCI UART
            std::array<uint8_t, receiveBufferSize> uciReceiveBuffer = {};
            auto nUciReceivedBytes =
                uciUart.read(uciReceiveBuffer.begin(), uciReceiveBuffer.size());

            if(nUciReceivedBytes > 0)
            {
                auto trimmedUciMessage = std::span{uciReceiveBuffer.begin(), nUciReceivedBytes};
                // Reflect to UCI UART
                hal::WriteTo(&uciUart, trimmedUciMessage);
            }
        }
    }
};


auto const uartReadTest = UartReadTest();
}
