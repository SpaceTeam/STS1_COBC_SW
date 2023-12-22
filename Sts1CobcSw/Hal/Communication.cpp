#include <Sts1CobcSw/Hal/Communication.hpp>


namespace sts1cobcsw::hal
{
auto Initialize(RODOS::HAL_SPI * spi, std::uint32_t baudRate) -> std::int32_t
{
    return spi->init(baudRate, /*slave=*/false, /*tiMode=*/false);
}


auto Initialize(RODOS::HAL_UART * uart, std::uint32_t baudRate) -> std::int32_t
{
    auto errorCode = uart->init(baudRate);
    uart->config(RODOS::UART_PARAMETER_ENABLE_DMA, RODOS::UART_BUF_SIZE);
    return errorCode;
}
}
