#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::hal
{
auto Initialize(RODOS::HAL_UART * uart, std::uint32_t baudRate) -> void;

template<typename T, std::size_t extent>
auto WriteTo(RODOS::HAL_UART * uart, std::span<T const, extent> data) -> void;

template<typename T, std::size_t extent>
auto WriteTo(RODOS::HAL_UART * uart, std::span<T const, extent> data, std::int64_t timeout) -> void;

template<typename T, std::size_t extent>
auto ReadFrom(RODOS::HAL_UART * uart, std::span<T, extent> data) -> void;

template<typename T, std::size_t extent>
auto ReadFrom(RODOS::HAL_UART * uart, std::span<T, extent> data, std::int64_t timeout) -> void;


namespace internal
{
template<typename T>
auto WriteTo(RODOS::HAL_UART * uart, std::span<T const> data) -> void;

template<typename T>
auto WriteTo(RODOS::HAL_UART * uart, std::span<T const> data, std::int64_t timeout) -> void;

template<typename T>
auto ReadFrom(RODOS::HAL_UART * uart, std::span<T> data) -> void;

template<typename T>
auto ReadFrom(RODOS::HAL_UART * uart, std::span<T> data, std::int64_t timeout) -> void;
}
}


#include <Sts1CobcSw/Hal/Uart.ipp>
