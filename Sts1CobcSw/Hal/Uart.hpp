#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>

#include <rodos_no_using_namespace.h>

#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::hal
{
// When removing namespace hal, I think it would be best to put ErrorCode and Result<T> into its own
// uart namespace, because then we would use namespaces correct again (just for avoiding name
// collisions) and in the smallest possible scope. Also, this is coherent with the EDU error codes
// and future ones we will introduce for the file system, etc.
enum class ErrorCode
{
    timeout = 1
};


template<typename T>
using Result = outcome_v2::experimental::status_result<T, ErrorCode, RebootPolicy>;


auto Initialize(RODOS::HAL_UART * uart, std::uint32_t baudRate) -> void;

template<typename T, std::size_t extent>
auto WriteTo(RODOS::HAL_UART * uart, std::span<T const, extent> data) -> void;

template<typename T, std::size_t extent>
[[nodiscard]] auto WriteTo(RODOS::HAL_UART * uart,
                           std::span<T const, extent> data,
                           std::int64_t timeout) -> Result<void>;

template<typename T, std::size_t extent>
auto ReadFrom(RODOS::HAL_UART * uart, std::span<T, extent> data) -> void;

template<typename T, std::size_t extent>
[[nodiscard]] auto ReadFrom(RODOS::HAL_UART * uart, std::span<T, extent> data, std::int64_t timeout)
    -> Result<void>;
}


#include <Sts1CobcSw/Hal/Uart.ipp>
