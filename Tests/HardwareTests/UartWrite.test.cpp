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
auto greenLed = RODOS::HAL_GPIO(hal::ledPin);
auto eduUart = RODOS::HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);
auto uciUart = RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);

class UartWrite : public RODOS::StaticThread<>
{
    void init() override
    {
        hal::InitPin(greenLed, hal::PinType::output, false);
        eduUart.init();
        uciUart.init();
    }


    void run() override
    {
        using std::operator""sv;

        auto toggle = true;

        TIME_LOOP(0, 1000 * RODOS::MILLISECONDS)
        {
            // Toggle LED to ensure MCU activity
            hal::SetPin(greenLed, toggle);

            hal::WriteTo(&RODOS::uart_stdout, "Write to uart_stdout\n");
            hal::WriteTo(&eduUart, "Write to eduUart\n");
            hal::WriteTo(&uciUart, "Write to uciUart\n");
            PRINTF("Write with PRINTF\n");

            toggle = not toggle;
        }
    }
};


auto const uartWrite = UartWrite();
}
