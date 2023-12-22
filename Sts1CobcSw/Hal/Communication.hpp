#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>


namespace sts1cobcsw::hal
{
auto Initialize(RODOS::HAL_SPI * spi, std::uint32_t baudRate) -> std::int32_t;
auto Initialize(RODOS::HAL_UART * uart, std::uint32_t baudRate) -> std::int32_t;

// TODO: Remove extent to avoid code bloat?
template<typename T, std::size_t extent>
auto WriteTo(auto * communicationInterface, std::span<T const, extent> data) -> void;

template<std::size_t extent>
auto ReadFrom(auto * communicationInterface, std::span<Byte, extent> data) -> void;

template<std::size_t size>
auto ReadFrom(RODOS::HAL_SPI * spi) -> std::array<Byte, size>;
}


#include <Sts1CobcSw/Hal/Communication.ipp>
