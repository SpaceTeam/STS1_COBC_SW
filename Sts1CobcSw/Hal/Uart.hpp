#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <rodos_no_using_namespace.h>

#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::hal
{
auto Initialize(RODOS::HAL_UART * uart, std::uint32_t baudRate) -> void;
auto Deinitialize(RODOS::HAL_UART * uart) -> void;

template<typename T, std::size_t extent>
auto WriteTo(RODOS::HAL_UART * uart, std::span<T const, extent> data) -> void;

template<typename T, std::size_t extent>
[[nodiscard]] auto WriteTo(RODOS::HAL_UART * uart,
                           std::span<T const, extent> data,
                           Duration timeout) -> Result<void>;

template<typename T, std::size_t extent>
auto ReadFrom(RODOS::HAL_UART * uart, std::span<T, extent> data) -> void;

template<typename T, std::size_t extent>
[[nodiscard]] auto ReadFrom(RODOS::HAL_UART * uart, std::span<T, extent> data, Duration timeout)
    -> Result<void>;
}


#include <Sts1CobcSw/Hal/Uart.ipp>  // IWYU pragma: keep
