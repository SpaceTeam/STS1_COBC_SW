#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::hal
{
auto Initialize(RODOS::HAL_SPI * spi, std::uint32_t baudRate) -> void;

// TODO: Maybe remove extent to reduce code bloat, or probably build time since it is just a single
// call to the Rodos function, which is trivial to inline.
template<typename T, std::size_t extent>
auto WriteTo(RODOS::HAL_SPI * spi, std::span<T const, extent> data) -> void;

template<typename T, std::size_t extent>
auto ReadFrom(RODOS::HAL_SPI * spi, std::span<T, extent> data) -> void;
}


#include <Sts1CobcSw/Hal/Spi.ipp>
