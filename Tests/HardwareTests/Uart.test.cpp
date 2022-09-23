//! @file
//! @brief  A program for testing the two UARTs of the COBC.
//!
//! If you flash `HelloUart.bin` onto the COBC, the messages "Hello from UART1" and "Hello from
//! UART2" are alternately written every 500 ms to the COBC EDU UART and the COBC UCI UART,
//! respectively. Both UARTs use the same configuration: 115200 baud, 8 data bits, no parity, 1 stop
//! bit.

#include <Sts1CobcSw/Hal/Communication.hpp>
#include <Sts1CobcSw/Hal/Gpio.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>

#include <rodos_no_using_namespace.h>

#include <string_view>

namespace RODOS
{
// NOLINTNEXTLINE(readability-identifier-naming)
extern HAL_UART uart_stdout;
}


namespace sts1cobcsw
{
RODOS::HAL_GPIO greenLed(hal::ledPin);

RODOS::HAL_UART uart1(RODOS::UART_IDX1, RODOS::GPIO_009, RODOS::GPIO_010);

class HelloUart : public RODOS::StaticThread<>
{
    void init() override
    {
        hal::InitPin(greenLed, hal::PinType::output, false);
        constexpr auto baudrate = 9'600;
        uart1.init(baudrate);
    }


    void run() override
    {
        using std::operator""sv;

        auto toggle = true;

        TIME_LOOP(0, 1000 * RODOS::MILLISECONDS)
        {
            greenLed.setPins(static_cast<uint32_t>(toggle));
            if(toggle)
            {
                hal::WriteTo(&RODOS::uart_stdout, "Hello from uart2\n");
            }
            else
            {
                hal::WriteTo(&RODOS::uart_stdout, "Hello from uart1\n");
            }
            toggle = not toggle;
        }
    }
};


auto const helloUart = HelloUart();
}
