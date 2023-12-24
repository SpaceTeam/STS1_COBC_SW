#pragma once


#include <Sts1CobcSw/Hal/Uart.hpp>


namespace sts1cobcsw::hal
{
template<typename T, std::size_t extent>
inline auto WriteTo(RODOS::HAL_UART * uart, std::span<T const, extent> data) -> void
{
    internal::WriteTo(uart, std::span<T const, std::dynamic_extent>(data));
}


template<typename T, std::size_t extent>
inline auto WriteTo(RODOS::HAL_UART * uart, std::span<T const, extent> data, std::int64_t timeout)
    -> void
{
    internal::WriteTo(uart, std::span<T const, std::dynamic_extent>(data), timeout);
}


template<typename T, std::size_t extent>
inline auto ReadFrom(RODOS::HAL_UART * uart, std::span<T, extent> data) -> void
{
    internal::ReadFrom(uart, std::span<T, std::dynamic_extent>(data));
}


template<typename T, std::size_t extent>
inline auto ReadFrom(RODOS::HAL_UART * uart, std::span<T, extent> data, std::int64_t timeout)
    -> void
{
    internal::ReadFrom(uart, std::span<T, std::dynamic_extent>(data), timeout);
}


namespace internal
{
// These internal functions use a span of dynamic extent to hopefully reduce binary size
template<typename T>
auto WriteTo(RODOS::HAL_UART * uart, std::span<T const> data) -> void
{
    auto bytes = std::as_bytes(data);
    std::size_t nWrittenBytes = 0U;
    while(nWrittenBytes < bytes.size())
    {
        // uart.write() writes at most RODOS::UART_BUF_SIZE bytes and returns how many it has
        // actually written. If a DMA transmit is already in progress it sends nothing but still
        // returns the number of bytes it would have written. Also, uart.write() returns -1 if the
        // UART_IDX is out of range. Since we can check that statically, we do not have to worry
        // about that though.
        nWrittenBytes += uart->write(bytes.data() + nWrittenBytes, bytes.size() - nWrittenBytes);
        while(not uart->isWriteFinished()) {}
    }
}


template<typename T>
auto WriteTo(RODOS::HAL_UART * uart, std::span<T const> data, std::int64_t timeout) -> void
{
    auto bytes = std::as_bytes(data);
    std::size_t nWrittenBytes = 0U;
    auto reactivationTime = RODOS::NOW() + timeout;
    while(nWrittenBytes < bytes.size())
    {
        // uart.write() writes at most RODOS::UART_BUF_SIZE bytes and returns how many it has
        // actually written. If a DMA transmit is already in progress it sends nothing but still
        // returns the number of bytes it would have written. Also, uart.write() returns -1 if the
        // UART_IDX is out of range. Since we can check that statically, we do not have to worry
        // about that though.
        nWrittenBytes += uart->write(bytes.data() + nWrittenBytes, bytes.size() - nWrittenBytes);
        uart->suspendUntilWriteFinished(reactivationTime);
        if(RODOS::NOW() >= reactivationTime)
        {
            // Timeout error
            return;
        }
    }
}


template<typename T>
auto ReadFrom(RODOS::HAL_UART * uart, std::span<T> data) -> void
{
    auto bytes = std::as_writable_bytes(data);
    std::size_t nReadBytes = 0U;
    while(nReadBytes < bytes.size())
    {
        while(not uart->isDataReady()) {}
        // uart.read() reads at most RODOS::UART_BUF_SIZE bytes and returns how many it has actually
        // read. If a DMA receive is already in progress it reads nothing but still returns the
        // number of bytes it would have read.
        nReadBytes += uart->read(bytes.data() + nReadBytes, bytes.size() - nReadBytes);
    }
}


template<typename T>
auto ReadFrom(RODOS::HAL_UART * uart, std::span<T> data, std::int64_t timeout) -> void
{
    auto bytes = std::as_writable_bytes(data);
    std::size_t nReadBytes = 0U;
    auto reactivationTime = RODOS::NOW() + timeout;
    while(nReadBytes < bytes.size())
    {
        uart->suspendUntilDataReady(reactivationTime);
        if(RODOS::NOW() >= reactivationTime)
        {
            // Timeout error
            return;
        }
        // uart.read() reads at most RODOS::UART_BUF_SIZE bytes and returns how many it has actually
        // read. If a DMA receive is already in progress it reads nothing but still returns the
        // number of bytes it would have read.
        nReadBytes += uart->read(bytes.data() + nReadBytes, bytes.size() - nReadBytes);
    }
}
}
}