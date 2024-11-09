#pragma once


// IWYU pragma: private, include <Sts1CobcSw/Hal/Spi.hpp>

#include <Sts1CobcSw/Hal/Spi.hpp>


namespace sts1cobcsw::hal
{
template<typename T, std::size_t extent>
auto WriteTo(Spi * spi, std::span<T const, extent> data, Duration timeout) -> void
{
    spi->Write(data.data(), data.size_bytes(), timeout);
}


template<typename T, std::size_t extent>
auto ReadFrom(Spi * spi, std::span<T, extent> data, Duration timeout) -> void
{
    spi->Read(data.data(), data.size_bytes(), timeout);
}
}
