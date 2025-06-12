#pragma once


#include <Sts1CobcSw/Hal/Uart.hpp>

#include <Sts1CobcSw/RodosTime/RodosTime.hpp>


namespace sts1cobcsw::hal
{
inline auto Deinitialize(RODOS::HAL_UART * uart) -> void
{
    uart->reset();
}


template<typename T, std::size_t extent>
auto WriteTo(RODOS::HAL_UART * uart, std::span<T const, extent> data) -> void
{
    auto bytes = std::as_bytes(data);
    std::size_t nWrittenBytes = 0;
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


template<typename T, std::size_t extent>
auto WriteTo(RODOS::HAL_UART * uart, std::span<T const, extent> data, Duration timeout)
    -> Result<void>
{
    auto bytes = std::as_bytes(data);
    std::size_t nWrittenBytes = 0;
    auto reactivationTime = CurrentRodosTime() + timeout;
    while(nWrittenBytes < bytes.size())
    {
        // uart.write() writes at most RODOS::UART_BUF_SIZE bytes and returns how many it has
        // actually written. If a DMA transmit is already in progress it sends nothing but still
        // returns the number of bytes it would have written. Also, uart.write() returns -1 if the
        // UART_IDX is out of range. Since we can check that statically, we do not have to worry
        // about that though.
        nWrittenBytes += uart->write(bytes.data() + nWrittenBytes, bytes.size() - nWrittenBytes);
        uart->suspendUntilWriteFinished(value_of(reactivationTime));
        if(CurrentRodosTime() >= reactivationTime)
        {
            return ErrorCode::timeout;
        }
    }
    return outcome_v2::success();
}


template<typename T, std::size_t extent>
auto ReadFrom(RODOS::HAL_UART * uart, std::span<T, extent> data) -> void
{
    auto bytes = std::as_writable_bytes(data);
    std::size_t nReadBytes = 0;
    while(nReadBytes < bytes.size())
    {
        while(not uart->isDataReady()) {}
        // uart.read() reads at most RODOS::UART_BUF_SIZE bytes and returns how many it has actually
        // read. If a DMA receive is already in progress it reads nothing but still returns the
        // number of bytes it would have read.
        nReadBytes += uart->read(bytes.data() + nReadBytes, bytes.size() - nReadBytes);
    }
}


template<typename T, std::size_t extent>
auto ReadFrom(RODOS::HAL_UART * uart, std::span<T, extent> data, Duration timeout) -> Result<void>
{
    auto bytes = std::as_writable_bytes(data);
    std::size_t nReadBytes = 0;
    auto reactivationTime = CurrentRodosTime() + timeout;
    while(nReadBytes < bytes.size())
    {
        uart->suspendUntilDataReady(value_of(reactivationTime));
        if(CurrentRodosTime() >= reactivationTime)
        {
            return ErrorCode::timeout;
        }
        // uart.read() reads at most RODOS::UART_BUF_SIZE bytes and returns how many it has actually
        // read. If a DMA receive is already in progress it reads nothing but still returns the
        // number of bytes it would have read.
        nReadBytes += uart->read(bytes.data() + nReadBytes, bytes.size() - nReadBytes);
    }
    return outcome_v2::success();
}
}
