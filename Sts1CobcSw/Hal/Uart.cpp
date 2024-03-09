#include <Sts1CobcSw/Hal/Uart.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw::hal
{
auto Initialize(RODOS::HAL_UART * uart, std::uint32_t baudRate) -> void
{
    // uart.init() only returns -1 if the UART_IDX is out of range. Since we can check that
    // statically we do not need to report that error at runtime.
    uart->init(baudRate);
    // uart->config(UART_PARAMETER_ENABLE_DMA) only returns -1 if the UART does not support DMA.
    // Since all UARTs that we use do support DMA this never happens and we have no errors to
    // report. Also, the UART buffer size is capped at RODOS::UART_BUF_SIZE.
    uart->config(RODOS::UART_PARAMETER_ENABLE_DMA, RODOS::UART_BUF_SIZE);
}
}
